#include "private.h"
#include "schedui.h"
#include "offl_cpp.h"
#include "dialmon.h"    // For WAITCURSOR
#include "shellids.h"   // For Help IDs

//xnotfmgr - can probably nuke most of this file

#define TF_DLGCTL                   TF_ALWAYS
#define TF_DUMPTRIGGER              0

#define MAX_GROUPNAME_LEN           40
#define MAX_LOADSTRING_LEN          64
#define MAX_SHORTTIMEFORMAT_LEN     80
#define MAX_TIMESEP_LEN             8

/////////////////////////////////////////////////////////////////////////////
// Design constants
/////////////////////////////////////////////////////////////////////////////
#define TASK_ALLDAYS                    (TASK_SUNDAY | TASK_MONDAY | TASK_TUESDAY | TASK_WEDNESDAY | TASK_THURSDAY | TASK_FRIDAY | TASK_SATURDAY)
#define TASK_WEEKDAYS                   (TASK_MONDAY | TASK_TUESDAY | TASK_WEDNESDAY | TASK_THURSDAY | TASK_FRIDAY)
#define TASK_WEEKENDDAYS                (TASK_SUNDAY | TASK_SATURDAY)
#define TASK_ALLMONTHS                  (TASK_JANUARY | TASK_FEBRUARY | TASK_MARCH | TASK_APRIL | TASK_MAY | TASK_JUNE | TASK_JULY | TASK_AUGUST | TASK_SEPTEMBER | TASK_OCTOBER | TASK_NOVEMBER | TASK_DECEMBER)

#define DEFAULT_DAILY_EVERYNDAYS        1
#define DEFAULT_DAILY_EVERYNDAYS_MIN    1
#define DEFAULT_DAILY_EVERYNDAYS_MAX    999
#define DEFAULT_WEEKLY_REPEATWEEKS      1
#define DEFAULT_WEEKLY_REPEATWEEKS_MIN  1
#define DEFAULT_WEEKLY_REPEATWEEKS_MAX  99
#define DEFAULT_MONTHLY_DAY_MIN         1
#define DEFAULT_MONTHLY_DAY_MAX         31
#define DEFAULT_MONTHLY_MONTHS          1
#define DEFAULT_MONTHLY_MONTHS_MIN      1
#define DEFAULT_MONTHLY_MONTHS_MAX      6
#define DEFAULT_UPDATETIME_HRS          0   // 12:00am
#define DEFAULT_UPDATETIME_MINS         0
#define DEFAULT_REPEATUPDATE_HRS        1
#define DEFAULT_REPEATUPDATE_HRS_MIN    1
#define DEFAULT_REPEATUPDATE_HRS_MAX    99
#define DEFAULT_REPEAT_START_HRS        9   // 9:00am
#define DEFAULT_REPEAT_START_MINS       0
#define DEFAULT_REPEAT_END_HRS          17  // 5:00pm
#define DEFAULT_REPEAT_END_MINS         0

#define DEFAULT_RANDOM_MINUTES_INTERVAL 30

// NOTE: These #defines have a dependency on the numeric order of the
// controls in the dialog resource! Make sure they are contiguious
// in RESOURCE.H! [jaym]
#define IDC_TYPE_GROUP_FIRST        IDC_CUSTOM_DAILY
#define IDC_TYPE_GROUP_LAST         IDC_CUSTOM_MONTHLY
    // Group type radio buttons

#define IDC_DAILY_GROUP_FIRST       IDC_CUSTOM_DAILY_EVERYNDAYS
#define IDC_DAILY_GROUP_LAST        IDC_CUSTOM_DAILY_EVERYWEEKDAY
    // Daily group type radio buttons

#define IDC_MONTHLY_GROUP_FIRST     IDC_CUSTOM_MONTHLY_DAYOFMONTH
#define IDC_MONTHLY_GROUP_LAST      IDC_CUSTOM_MONTHLY_PERIODIC
    // Monthly group type radio buttons

#define IDC_TIME_GROUP_FIRST        IDC_CUSTOM_TIME_UPDATEAT
#define IDC_TIME_GROUP_LAST         IDC_CUSTOM_TIME_REPEATEVERY
    // Time radio buttons

#define IDC_CUSTOM_DAILY_FIRST      IDC_CUSTOM_DAILY_EVERYNDAYS
#define IDC_CUSTOM_DAILY_LAST       IDC_CUSTOM_DAILY_STATIC1
#define IDC_CUSTOM_WEEKLY_FIRST     IDC_CUSTOM_WEEKLY_STATIC1
#define IDC_CUSTOM_WEEKLY_LAST      IDC_CUSTOM_WEEKLY_DAY7
#define IDC_CUSTOM_MONTHLY_FIRST    IDC_CUSTOM_MONTHLY_DAYOFMONTH
#define IDC_CUSTOM_MONTHLY_LAST     IDC_CUSTOM_MONTHLY_STATIC4
    // Group type controls

#define IDS_MONTHLY_LIST1_FIRST     IDS_WEEK1
#define IDS_MONTHLY_LIST1_LAST      IDS_WEEK5
    // Combo box items.

#define CX_DIALOG_GUTTER            10
#define CY_DIALOG_GUTTER            10

/////////////////////////////////////////////////////////////////////////////
// Module variables
/////////////////////////////////////////////////////////////////////////////
static const TCHAR s_szRepeatHrsAreMins[] = TEXT("RepeatHrsAreMins");

static const CTLGRPITEM c_rgnCtlGroups[] = 
{
    // 'Container' control                  First item in control       Last item in control
    //-------------------------             --------------------------- ----------------------
    { IDC_CUSTOM_GROUP_DAILY,               IDC_CUSTOM_DAILY_FIRST,     IDC_CUSTOM_DAILY_LAST },
    { IDC_CUSTOM_GROUP_WEEKLY,              IDC_CUSTOM_WEEKLY_FIRST,    IDC_CUSTOM_WEEKLY_LAST },
    { IDC_CUSTOM_GROUP_MONTHLY,             IDC_CUSTOM_MONTHLY_FIRST,   IDC_CUSTOM_MONTHLY_LAST },
};

static const CTLGRPITEM c_rgnCtlItems[] = 
{
    // 'Container' control                  First item in control       Last item in control
    //-------------------------             --------------------------- ----------------------
    { IDC_CUSTOM_MONTHLY_PERIODIC_LIST1,    IDS_MONTHLY_LIST1_FIRST,    IDS_MONTHLY_LIST1_LAST },
};

static const WORD c_rgwMonthMaps[] =
{
    0x0FFF,         // every month       111111111111b
    0x0AAA,         // every other month 101010101010b
    0x0924,         // every 3 months    100100100100b
    0x0888,         // every 4 months    100010001000b
    0x0842,         // every 5 months    100001000010b
    0x0820          // every 6 months    100000100000b
};

#define NUM_CUSTOM_DAYSOFTHEWEEK    3
static WORD s_rgwDaysOfTheWeek[NUM_CUSTOM_DAYSOFTHEWEEK+7] = 
{
    TASK_ALLDAYS,
    TASK_WEEKDAYS,
    TASK_WEEKENDDAYS,

    // ...
};
    // NOTE: These should be kept in line with IDS_DAY1 to IDS_DAY3

extern TCHAR c_szHelpFile[];
DWORD aCustomDlgHelpIDs[] =
{
    IDD_CUSTOM_SCHEDULE,                        IDH_CUSTOM_SCHEDULE,

    IDC_CUSTOM_NEWGROUP,                        IDH_SCHEDULE_NEW,
    IDC_CUSTOM_REMOVEGROUP,                     IDH_SCHEDULE_REMOVE,

    IDC_CUSTOM_GROUP_LIST,                      IDH_NEW_NAME,
    IDC_CUSTOM_GROUP_EDIT,                      IDH_NEW_NAME,

    IDC_CUSTOM_DAILY,                           IDH_SCHED_DAYS,
    IDC_CUSTOM_WEEKLY,                          IDH_SCHED_DAYS,
    IDC_CUSTOM_MONTHLY,                         IDH_SCHED_DAYS,

    IDC_CUSTOM_DAILY_EVERYNDAYS,                IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_DAILY_EVERYNDAYS_EDIT,           IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_DAILY_STATIC1,                   IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_DAILY_EVERYWEEKDAY,              IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_WEEKLY_STATIC1,                  IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_WEEKLY_REPEATWEEKS_EDIT,         IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_WEEKLY_STATIC2,                  IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_WEEKLY_DAY1,                     IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_WEEKLY_DAY2,                     IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_WEEKLY_DAY3,                     IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_WEEKLY_DAY4,                     IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_WEEKLY_DAY5,                     IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_WEEKLY_DAY6,                     IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_WEEKLY_DAY7,                     IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_MONTHLY_DAYOFMONTH,              IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_MONTHLY_STATIC3,                 IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_MONTHLY_DAYOFMONTH_MONTH_EDIT,   IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_MONTHLY_STATIC4,                 IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_MONTHLY_PERIODIC,                IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_MONTHLY_PERIODIC_LIST1,          IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_MONTHLY_PERIODIC_LIST2,          IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_MONTHLY_STATIC1,                 IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_MONTHLY_PERIODIC_EDIT,           IDH_SCHED_FREQUENCY,
    IDC_CUSTOM_MONTHLY_STATIC2,                 IDH_SCHED_FREQUENCY,

    IDC_CUSTOM_TIME_UPDATEAT,                   IDH_SCHED_TIME,
    IDC_CUSTOM_TIME_UPDATEAT_TIME,              IDH_SCHED_TIME,

    IDC_CUSTOM_TIME_REPEATEVERY,                IDH_SCHED_REPEAT,
    IDC_CUSTOM_TIME_REPEATEVERY_EDIT,           IDH_SCHED_REPEAT,
    IDC_CUSTOM_TIME_REPEATBETWEEN,              IDH_SCHED_REPEAT,
    IDC_CUSTOM_TIME_REPEATBETWEEN_START_TIME,   IDH_SCHED_REPEAT,
    IDC_CUSTOM_TIME_REPEATBETWEEN_END_TIME,     IDH_SCHED_REPEAT,

    IDC_CUSTOM_MINIMIZENETUSE,                  IDH_VARY_START
};
    // Help IDs

/////////////////////////////////////////////////////////////////////////////
// Local functions
/////////////////////////////////////////////////////////////////////////////
void InitDlgCtls(HWND hwndDlg, SSUIDLGINFO * pDlgInfo);
void PositionDlgCtls(HWND hwndDlg);
void InitDlgDefaults(HWND hwndDlg, SSUIDLGINFO * pDlgInfo);
void InitDlgInfo(HWND hwndDlg, SSUIDLGINFO * pDlgInfo);

BOOL HandleDlgButtonClick(HWND hwndDlg, WPARAM wParam, SSUIDLGINFO * pDlgInfo);
BOOL HandleButtonClick(HWND hwndDlg, WPARAM wParam);
BOOL HandleGroupChange(HWND hwndDlg, SSUIDLGINFO * pDlgInfo);

void SetDlgCustomType(HWND hwndDlg, int iType);
void SetDlgWeekDays(HWND hwndDlg, WORD rgfDaysOfTheWeek);
WORD GetDlgWeekDays(HWND hwndDlg);
void SetTaskTriggerToDefaults(TASK_TRIGGER * pTaskTrigger);
void SetTaskTriggerFromDlg(HWND hwndDlg, SSUIDLGINFO * pDlgInfo);

HRESULT CreateScheduleGroupFromDlg(HWND hwndDlg, SSUIDLGINFO * pDlgInfo);

void    OnDataChanged(HWND hwndDlg, SSUIDLGINFO * pDlgInfo);
HRESULT ApplyDlgChanges(HWND hwndDlg, SSUIDLGINFO * pDlgInfo);
BOOL    ValidateDlgFields(HWND hwndDlg, SSUIDLGINFO * pDlgInfo);

UINT    GetDlgGroupName(HWND hwndDlg, LPTSTR pszGroupName, int cchGroupNameLen);
int     GetDlgGroupNameLength(HWND hwndDlg);
BOOL    SetDlgItemMonth(HWND hwndDlg, int idCtl, WORD rgfMonths);
WORD    GetDlgItemMonth(HWND hwndDlg, int idCtl);
void    SetDlgItemNextUpdate(HWND hwndDlg, int idCtl, SSUIDLGINFO * pDlgInfo);

HRESULT GetSchedGroupTaskTrigger(PNOTIFICATIONCOOKIE pGroupCookie,TASK_TRIGGER * pTaskTrigger);
BOOL CALLBACK ShowScheduleUIDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

DWORD   IncTime(DWORD dwTime, int iIncHours, int iIncMinutes);
int     DayFromDaysFlags(DWORD rgfDays);
int     MonthCountFromMonthsFlags(WORD rgfMonths);
int     DayIndexFromDaysOfTheWeekFlags(WORD rgfDaysOfTheWeek);
HRESULT GetDaysOfWeekString(WORD rgfDaysOfTheWeek, LPTSTR ptszBuf, UINT cchBuf);
HRESULT GetMonthsString(WORD rgfMonths, LPTSTR ptszBuf, UINT cchBuf);
void    RestrictDlgItemRange(HWND hwndDlg, int idCtl, SSUIDLGINFO * pDlgInfo);
void    RestrictTimeRepeatEvery(HWND hwndDlg);

int     SGMessageBox(HWND hwndParent, UINT idStringRes, UINT uType);

#ifdef DEBUG
void    DumpTaskTrigger(TASK_TRIGGER * pTaskTrigger);
#endif  // DEBUG

/////////////////////////////////////////////////////////////////////////////
// ScheduleSummaryFromGroup
/////////////////////////////////////////////////////////////////////////////
HRESULT ScheduleSummaryFromGroup
(
    /* [in] */      PNOTIFICATIONCOOKIE pGroupCookie,
    /* [in][out] */ LPTSTR              pszSummary,
    /* [in] */      UINT                cchSummary
)
{
//xnotfmgr

    HRESULT hrResult;

    ASSERT((pszSummary != NULL) && (cchSummary > 0));

    for (;;)
    {
         *pszSummary = TEXT('\0');
 
        TASK_TRIGGER    tt;
        if (FAILED(hrResult = GetSchedGroupTaskTrigger(pGroupCookie, &tt)))
            break;

        if (FAILED(hrResult = ScheduleSummaryFromTaskTrigger(&tt, pszSummary, cchSummary)))
            break;

        hrResult = S_OK;
        break;
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// ScheduleSummaryFromTaskTrigger
/////////////////////////////////////////////////////////////////////////////
HRESULT ScheduleSummaryFromTaskTrigger
(
    TASK_TRIGGER *  pTT,
    LPTSTR          pszSummary,
    UINT            cchSummary
)
{
    TCHAR   szLineOut[128];
    TCHAR   szFormat[128];
    HRESULT hrResult;

    if (!MLLoadString(IDS_SUMMARY_UPDATE, szLineOut, ARRAYSIZE(szLineOut)))
    {
        hrResult = HRESULT_FROM_WIN32(GetLastError());
        return hrResult;
    }

    StrNCpy(pszSummary, szLineOut, cchSummary);

    switch (pTT->TriggerType)
    {
        case TASK_TIME_TRIGGER_DAILY:
        {
            if (pTT->Type.Daily.DaysInterval == 1)
            {
                // Daily -- every day
                if (!MLLoadString(IDS_SUMMARY_EVERY_DAY, szLineOut, ARRAYSIZE(szLineOut)))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }
            }
            else
            {
                // Daily -- every %d days
                if (!MLLoadString(IDS_SUMMARY_DAILY_FORMAT, szFormat, ARRAYSIZE(szFormat)))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }

                wnsprintf(szLineOut, ARRAYSIZE(szLineOut), szFormat, 
                          pTT->Type.Daily.DaysInterval);
            }

            lstrcatn(pszSummary, szLineOut, cchSummary);
            break;
        }

        case TASK_TIME_TRIGGER_WEEKLY:
        {
            TCHAR szDOW[128];

            if (FAILED(hrResult = GetDaysOfWeekString(  pTT->Type.Weekly.rgfDaysOfTheWeek,
                                                        szDOW,
                                                        ARRAYSIZE(szDOW))))
            {
                return hrResult;
            }

            if (pTT->Type.Weekly.WeeksInterval == 1)
            {
                // Daily -- every weekday
                if (pTT->Type.Weekly.rgfDaysOfTheWeek == TASK_WEEKDAYS)
                {
                    if (!MLLoadString(IDS_SUMMARY_EVERY_WEEKDAY, szLineOut, ARRAYSIZE(szLineOut)))
                    {
                        hrResult = HRESULT_FROM_WIN32(GetLastError());
                        return hrResult;
                    }
                }
                else
                {
                    // Weekly -- every %s of every week
                    if (!MLLoadString(IDS_SUMMARY_EVERY_WEEK_FORMAT, szFormat, ARRAYSIZE(szFormat)))
                    {
                        hrResult = HRESULT_FROM_WIN32(GetLastError());
                        return hrResult;
                    }

                    wnsprintf(szLineOut, ARRAYSIZE(szLineOut), szFormat, szDOW);
                }
            }
            else
            {
                TCHAR szWeeks[16];

                // Weekly -- every %s of every %s weeks
                if (!MLLoadString(IDS_SUMMARY_WEEKLY_FORMAT, szFormat, ARRAYSIZE(szFormat)))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }

                wnsprintf(szWeeks, ARRAYSIZE(szWeeks), "%d", pTT->Type.Weekly.WeeksInterval);

                TCHAR * rgpsz[2] = { szDOW, szWeeks };
                if (!FormatMessage( FORMAT_MESSAGE_FROM_STRING
                                        | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                    szFormat,
                                    0,
                                    0,
                                    szLineOut, 
                                    ARRAYSIZE(szLineOut),
                                    (va_list*)rgpsz))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }
            }

            lstrcatn(pszSummary, szLineOut, cchSummary);
            break;
        }

        case TASK_TIME_TRIGGER_MONTHLYDATE:
        {
            if (pTT->Type.MonthlyDate.rgfMonths == TASK_ALLMONTHS)
            {
                // Monthly -- on day %d of every month
                if (!MLLoadString(
                                IDS_SUMMARY_EVERY_MONTHLYDATE_FORMAT,
                                szFormat,
                                ARRAYSIZE(szFormat)))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }

                wnsprintf(szLineOut, ARRAYSIZE(szLineOut), szFormat, 
                          DayFromDaysFlags(pTT->Type.MonthlyDate.rgfDays));
            }
            else
            {
                // Monthly -- on day %d of %s
                if (!MLLoadString(
                                IDS_SUMMARY_MONTHLYDATE_FORMAT,
                                szFormat,
                                ARRAYSIZE(szFormat)))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }

                TCHAR szMonths[128];
                if (FAILED(hrResult = GetMonthsString(  pTT->Type.MonthlyDate.rgfMonths,
                                                        szMonths,
                                                        ARRAYSIZE(szMonths))))
                {
                    return hrResult;
                }

                TCHAR szDay[16];
                wnsprintf(szDay, ARRAYSIZE(szDay), "%d", 
                          DayFromDaysFlags(pTT->Type.MonthlyDate.rgfDays));

                TCHAR * rgpsz[2] = { szDay, szMonths };
                if (!FormatMessage( FORMAT_MESSAGE_FROM_STRING
                                        | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                    szFormat,
                                    0,
                                    0,
                                    szLineOut, 
                                    ARRAYSIZE(szLineOut),
                                    (va_list*)rgpsz))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }
            }

            lstrcatn(pszSummary, szLineOut, cchSummary);
            break;
        }

        case TASK_TIME_TRIGGER_MONTHLYDOW:
        {
            ASSERT((pTT->Type.MonthlyDOW.wWhichWeek >= TASK_FIRST_WEEK));
            ASSERT((pTT->Type.MonthlyDOW.wWhichWeek <= TASK_LAST_WEEK));

            TCHAR szWeek[32];
            if (!MLLoadString(
                            IDS_MONTHLY_LIST1_FIRST + pTT->Type.MonthlyDOW.wWhichWeek - 1,
                            szWeek,
                            ARRAYSIZE(szWeek)))
            {
                hrResult = HRESULT_FROM_WIN32(GetLastError());
                return hrResult;
            }

            TCHAR szDOW[128];
            if (FAILED(hrResult = GetDaysOfWeekString(  pTT->Type.MonthlyDOW.rgfDaysOfTheWeek,
                                                        szDOW,
                                                        ARRAYSIZE(szDOW))))
            {
                return hrResult;
            }

            if (pTT->Type.MonthlyDate.rgfMonths == TASK_ALLMONTHS)
            {
                // Monthly -- on the %s %s of every month
                if (!MLLoadString(
                                IDS_SUMMARY_EVERY_MONTHLYDOW_FORMAT,
                                szFormat,
                                ARRAYSIZE(szFormat)))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }

                TCHAR * rgpsz[2] = { szWeek, szDOW };
                if (!FormatMessage( FORMAT_MESSAGE_FROM_STRING
                                        | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                    szFormat,
                                    0,
                                    0,
                                    szLineOut, 
                                    ARRAYSIZE(szLineOut),
                                    (va_list*)rgpsz))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }
            }
            else
            {
                TCHAR szMonths[128];
                if (FAILED(hrResult = GetMonthsString(  pTT->Type.MonthlyDOW.rgfMonths,
                                                        szMonths,
                                                        ARRAYSIZE(szMonths))))
                {
                    return hrResult;
                }

                // Monthly -- on the %s %s of %s
                if (!MLLoadString(
                                IDS_SUMMARY_MONTHLYDOW_FORMAT,
                                szFormat,
                                ARRAYSIZE(szFormat)))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }

                TCHAR * rgpsz[3] = { szWeek, szDOW, szMonths };
                if (!FormatMessage( FORMAT_MESSAGE_FROM_STRING
                                        | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                    szFormat,
                                    0,
                                    0,
                                    szLineOut, 
                                    ARRAYSIZE(szLineOut),
                                    (va_list*)rgpsz))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }
            }

            lstrcatn(pszSummary, szLineOut, cchSummary);
            break;
        }

        default:
        {
            ASSERT(FALSE);
            return E_FAIL;
        }
    }

    TCHAR       szStartTime[MAX_SHORTTIMEFORMAT_LEN];
    SYSTEMTIME  st = { 0 };
    st.wHour    = pTT->wStartHour;
    st.wMinute  = pTT->wStartMinute;

    EVAL(GetTimeFormat( LOCALE_USER_DEFAULT,
                        TIME_NOSECONDS,
                        &st,
                        NULL,
                        szStartTime,
                        ARRAYSIZE(szStartTime)) > 0);

    // Handle the 'Time' settings.
    if (pTT->MinutesInterval == 0)
    {
        // at %s
        if (!MLLoadString(
                        IDS_SUMMARY_ONCE_DAY_FORMAT,
                        szFormat,
                        ARRAYSIZE(szFormat)))
        {
            hrResult = HRESULT_FROM_WIN32(GetLastError());
            return hrResult;
        }

        wnsprintf(szLineOut, ARRAYSIZE(szLineOut), szFormat, szStartTime);
        lstrcatn(pszSummary, szLineOut, cchSummary);
    }
    else
    {
        if ((pTT->MinutesInterval % 60) == 0)   //repeating on the hour
        {
            if ((pTT->MinutesInterval / 60) == 1)
            {
                // every hour
                if (!MLLoadString(
                                IDS_SUMMARY_REPEAT_EVERY_HOUR,
                                szLineOut,
                                ARRAYSIZE(szLineOut)))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }
            }
            else
            {
                // every %d hours
                if (!MLLoadString(
                                IDS_SUMMARY_REPEAT_HOURLY_FORMAT,
                                szFormat,
                                ARRAYSIZE(szFormat)))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }

                wnsprintf(szLineOut, ARRAYSIZE(szLineOut), szFormat, 
                         (pTT->MinutesInterval / 60));
            }
        }
        else        //not integral hours, use minutes
        {
            if (pTT->MinutesInterval == 1)
            {
                // every minute
                if (!MLLoadString(
                                IDS_SUMMARY_REPEAT_EVERY_MIN,
                                szLineOut,
                                ARRAYSIZE(szLineOut)))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }
            }
            else
            {
                // every %d minutes
                if (!MLLoadString(
                                IDS_SUMMARY_REPEAT_MINUTELY_FORMAT,
                                szFormat,
                                ARRAYSIZE(szFormat)))
                {
                    hrResult = HRESULT_FROM_WIN32(GetLastError());
                    return hrResult;
                }

                wnsprintf(szLineOut, ARRAYSIZE(szLineOut), szFormat, 
                         pTT->MinutesInterval);
            }
        }

        lstrcatn(pszSummary, szLineOut, cchSummary);

        // between %s and %s
        if (pTT->MinutesDuration != (24 * 60))
        {
            if (!MLLoadString(
                            IDS_SUMMARY_REPEAT_BETWEEN_FORMAT,
                            szFormat,
                            ARRAYSIZE(szFormat)))
            {
                hrResult = HRESULT_FROM_WIN32(GetLastError());
                return hrResult;
            }

            DWORD dwEndTime = IncTime(  MAKELONG(pTT->wStartHour, pTT->wStartMinute),
                                        (pTT->MinutesDuration / 60),
                                        (pTT->MinutesDuration % 60));

            ZeroMemory(&st, SIZEOF(SYSTEMTIME));
            st.wHour    = LOWORD(dwEndTime);
            st.wMinute  = HIWORD(dwEndTime);

            TCHAR szEndTime[MAX_SHORTTIMEFORMAT_LEN];
            EVAL(GetTimeFormat( LOCALE_USER_DEFAULT,
                                TIME_NOSECONDS,
                                &st,
                                NULL,
                                szEndTime,
                                ARRAYSIZE(szEndTime)) > 0);

            TCHAR * rgpsz[2] = { szStartTime, szEndTime };
            if (!FormatMessage( FORMAT_MESSAGE_FROM_STRING
                                    | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                szFormat,
                                0,
                                0,
                                szLineOut, 
                                ARRAYSIZE(szLineOut),
                                (va_list*)rgpsz))
            {
                hrResult = HRESULT_FROM_WIN32(GetLastError());
                return hrResult;
            }

            lstrcatn(pszSummary, szLineOut, cchSummary);
        }
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// GetDaysOfWeekString
/////////////////////////////////////////////////////////////////////////////
HRESULT GetDaysOfWeekString
(
    WORD    rgfDaysOfTheWeek,
    LPTSTR  ptszBuf,
    UINT    cchBuf
)
{
    if (rgfDaysOfTheWeek == 0)
        return E_INVALIDARG;

    int     cch;
    TCHAR   tszSep[8];
    BOOL    fMoreThanOne = FALSE;
    HRESULT hrResult;


    // REVIEW: Could probably make this simpler by creating a table [jaym]
    // TASK_* -> LOCALE_SABBREVDAYNAME?
    // BUGBUG duh...this is gross

    if (!MLLoadString(IDS_SUMMARY_LIST_SEP, tszSep, ARRAYSIZE(tszSep)))
    {
        hrResult = HRESULT_FROM_WIN32(GetLastError());
        return hrResult;
    }

    *ptszBuf = TEXT('\0');

    if (rgfDaysOfTheWeek & TASK_MONDAY)
    {
        if (!GetLocaleInfo( LOCALE_USER_DEFAULT,
                            LOCALE_SABBREVDAYNAME1,
                            ptszBuf,
                            cchBuf))
        {
            hrResult = HRESULT_FROM_WIN32(GetLastError());
            return hrResult;
        }

        fMoreThanOne = TRUE;
    }

    if (rgfDaysOfTheWeek & TASK_TUESDAY)
    {
        if (fMoreThanOne)
            lstrcat(ptszBuf, tszSep);
        
        cch = lstrlen(ptszBuf);
        if (!GetLocaleInfo( LOCALE_USER_DEFAULT,
                            LOCALE_SABBREVDAYNAME2,
                            ptszBuf + cch,
                            cchBuf - cch))
        {
            hrResult = HRESULT_FROM_WIN32(GetLastError());
            return hrResult;
        }

        fMoreThanOne = TRUE;
    }

    if (rgfDaysOfTheWeek & TASK_WEDNESDAY)
    {
        if (fMoreThanOne)
            lstrcat(ptszBuf, tszSep);
        
        cch = lstrlen(ptszBuf);
        if (!GetLocaleInfo( LOCALE_USER_DEFAULT,
                            LOCALE_SABBREVDAYNAME3,
                            ptszBuf + cch,
                            cchBuf - cch))
        {
            hrResult = HRESULT_FROM_WIN32(GetLastError());
            return hrResult;
        }

        fMoreThanOne = TRUE;
    }

    if (rgfDaysOfTheWeek & TASK_THURSDAY)
    {
        if (fMoreThanOne)
            lstrcat(ptszBuf, tszSep);
        
        cch = lstrlen(ptszBuf);
        if (!GetLocaleInfo( LOCALE_USER_DEFAULT,
                            LOCALE_SABBREVDAYNAME4,
                            ptszBuf + cch,
                            cchBuf - cch))
        {
            hrResult = HRESULT_FROM_WIN32(GetLastError());
            return hrResult;
        }
        
        fMoreThanOne = TRUE;
    }

    if (rgfDaysOfTheWeek & TASK_FRIDAY)
    {
        if (fMoreThanOne)
            lstrcat(ptszBuf, tszSep);

        cch = lstrlen(ptszBuf);
        if (!GetLocaleInfo( LOCALE_USER_DEFAULT,
                            LOCALE_SABBREVDAYNAME5,
                            ptszBuf + cch,
                            cchBuf - cch))
        {
            hrResult = HRESULT_FROM_WIN32(GetLastError());
            return hrResult;
        }

        fMoreThanOne = TRUE;
    }

    if (rgfDaysOfTheWeek & TASK_SATURDAY)
    {
        if (fMoreThanOne)
            lstrcat(ptszBuf, tszSep);

        cch = lstrlen(ptszBuf);
        if (!GetLocaleInfo( LOCALE_USER_DEFAULT,
                            LOCALE_SABBREVDAYNAME6,
                            ptszBuf + cch,
                            cchBuf - cch))
        {
            hrResult = HRESULT_FROM_WIN32(GetLastError());
            return hrResult;
        }

        fMoreThanOne = TRUE;
    }

    if (rgfDaysOfTheWeek & TASK_SUNDAY)
    {
        if (fMoreThanOne)
            lstrcat(ptszBuf, tszSep);

        cch = lstrlen(ptszBuf);
        if (!GetLocaleInfo( LOCALE_USER_DEFAULT,
                            LOCALE_SABBREVDAYNAME7,
                            ptszBuf + cch,
                            cchBuf - cch))
        {
            hrResult = HRESULT_FROM_WIN32(GetLastError());
            return hrResult;
        }
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// GetMonthsString
/////////////////////////////////////////////////////////////////////////////
HRESULT GetMonthsString
(
    WORD    rgfMonths,
    LPTSTR  ptszBuf,
    UINT    cchBuf
)
{
    if (rgfMonths == 0)
        return E_INVALIDARG;

    int     cch;
    TCHAR   tszSep[8];
    BOOL    fMoreThanOne = FALSE;
    HRESULT hr;

    if (!MLLoadString(IDS_SUMMARY_LIST_SEP, tszSep, ARRAYSIZE(tszSep)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    *ptszBuf = TEXT('\0');

    for (WORD i = 0; i < 12; i++)
    {
        if ((rgfMonths >> i) & 0x1)
        {
            if (fMoreThanOne)
                lstrcat(ptszBuf, tszSep);

            cch = lstrlen(ptszBuf);
            if (!GetLocaleInfo( LOCALE_USER_DEFAULT,
                                LOCALE_SABBREVMONTHNAME1 + i,
                                ptszBuf + cch,
                                cchBuf - cch))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                return hr;
            }

            fMoreThanOne = TRUE;
        }
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// ShowScheduleUI
/////////////////////////////////////////////////////////////////////////////
HRESULT ShowScheduleUI
(
    /* [in] */      HWND                hwndParent,
    /* [in][out] */ PNOTIFICATIONCOOKIE pGroupCookie,
    /* [in][out] */ DWORD *             pdwFlags
)
{
//xnotfmgr

    SSUIDLGINFO ssuiDlgInfo = { 0 };
    HRESULT     hrResult;

    // Initialize the custom time edit control class.
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_DATE_CLASSES;
    EVAL(SUCCEEDED(InitCommonControlsEx(&icc)));

    for (;;)
    {
        // REVIEW: Check for invalid/conflicting dwFlags. [jaym]
        if  (
            (pGroupCookie == NULL)
            ||
            (pdwFlags == NULL)
            )
        {
            return E_INVALIDARG;
        }

        ssuiDlgInfo.dwFlags         = *pdwFlags;
        ssuiDlgInfo.pGroupCookie    = pGroupCookie;

        // Get the GetRunTimes function entry point from URLMON.
        if ((ssuiDlgInfo.hinstURLMON = LoadLibrary(TEXT("URLMON.DLL"))) != NULL)
        {
            EVAL((ssuiDlgInfo.pfnGetRunTimes = (GRTFUNCTION)GetProcAddress( ssuiDlgInfo.hinstURLMON,
                                                                            TEXT("GetRunTimes"))) != NULL);
        }
        else
            TraceMsg(TF_ERROR, "Unable to LoadLibrary(URLMON.DLL)");

        // Setup the TASK_TRIGGER with default or sched. group info.
        if (ssuiDlgInfo.dwFlags & SSUI_CREATENEWSCHEDULE)
        {
            *(ssuiDlgInfo.pGroupCookie) = GUID_NULL;
            SetTaskTriggerToDefaults(&ssuiDlgInfo.ttTaskTrigger);
        }
        else
        {
            ASSERT(ssuiDlgInfo.dwFlags & SSUI_EDITSCHEDULE);

            if (FAILED(hrResult = GetSchedGroupTaskTrigger( ssuiDlgInfo.pGroupCookie,
                                                            &ssuiDlgInfo.ttTaskTrigger)))
            {
                hrResult = E_INVALIDARG;
                break;
            }
        }

        // See if we want to specify schedule hours in minutes.
        ReadRegValue(   HKEY_CURRENT_USER,
                        WEBCHECK_REGKEY,
                        s_szRepeatHrsAreMins,
                        &ssuiDlgInfo.dwRepeatHrsAreMins,
                        SIZEOF(&ssuiDlgInfo.dwRepeatHrsAreMins));

#ifdef DEBUG
        if (ssuiDlgInfo.dwRepeatHrsAreMins)
            TraceMsg(TF_ALWAYS, "!!! WARNING! Treating Repeat Hours as Minutes !!!");
#endif  // DEBUG

        // Show the dialog.
        int iResult;
        if ((iResult = DialogBoxParam(  MLGetHinst(),
                                        MAKEINTRESOURCE(IDD_CUSTOM_SCHEDULE),
                                        hwndParent,
                                        ShowScheduleUIDlgProc,
                                        (LPARAM)&ssuiDlgInfo)) < 0)
        {
            TraceMsg(TF_ERROR, "SSUI: DialogBoxParam failed");

            hrResult = E_FAIL;
            break;
        }

        switch (iResult)
        {
            case IDCANCEL:
            {
                if (!(ssuiDlgInfo.dwFlags & SSUI_EDITSCHEDULE))
                    break;

                // FALL THROUGH!!!
            }

            case IDOK:
            {
                *pdwFlags = ssuiDlgInfo.dwFlags;

                if (ssuiDlgInfo.dwFlags & SSUI_SCHEDULELISTUPDATED)
                {
                    ASSERT(ssuiDlgInfo.pGroupCookie != NULL);
                    ASSERT(*(ssuiDlgInfo.pGroupCookie) != GUID_NULL);

                    pGroupCookie = ssuiDlgInfo.pGroupCookie;
                }

                break;
            }

            default:
            {
                ASSERT(FALSE);
                break;
            }
        }

        hrResult = S_OK;
        break;
    }

    if (ssuiDlgInfo.hinstURLMON != NULL)
        FreeLibrary(ssuiDlgInfo.hinstURLMON);

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// ShowScheduleUIDlgProc
/////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK ShowScheduleUIDlgProc
(
    HWND    hwndDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    SSUIDLGINFO * pDlgInfo = (SSUIDLGINFO *)GetWindowLong(hwndDlg, DWL_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            SSUIDLGINFO * pDlgInfo = (SSUIDLGINFO *)lParam;
            ASSERT(pDlgInfo != NULL);

            SetWindowLong(hwndDlg, DWL_USER, lParam);

            // Resize the dialog -- controls are to the right in RC.
            RECT rectCancel;
            GetWindowRect(GetDlgItem(hwndDlg, IDCANCEL), &rectCancel);
            MapWindowPoints(HWND_DESKTOP, hwndDlg, (POINT *)&rectCancel, 2);

            SetWindowPos(   hwndDlg,
                            NULL,
                            0,
                            0,
                            rectCancel.right
                                + (GetSystemMetrics(SM_CXDLGFRAME) * 2)
                                + CX_DIALOG_GUTTER,
                            rectCancel.bottom
                                + (GetSystemMetrics(SM_CYDLGFRAME) * 2)
                                + GetSystemMetrics(SM_CYCAPTION)
                                + CY_DIALOG_GUTTER,
                            SWP_NOMOVE
                                | SWP_NOACTIVATE
                                | SWP_NOZORDER
                                | SWP_NOOWNERZORDER);

            // Initialize the initial dialog settings and content.
            InitDlgCtls(hwndDlg, pDlgInfo);
            InitDlgInfo(hwndDlg, pDlgInfo);

            break;
        }

        case WM_HELP:
        {
            MLWinHelpWrap((HWND)((LPHELPINFO) lParam)->hItemHandle,
                    c_szHelpFile,
                    HELP_WM_HELP,
                    (DWORD)aCustomDlgHelpIDs);
            break;
        }

        case WM_CONTEXTMENU:
        {
            MLWinHelpWrap((HWND)wParam,
                    c_szHelpFile,
                    HELP_CONTEXTMENU,
                    (DWORD)aCustomDlgHelpIDs);
            break;
        }

        case WM_COMMAND:
        {
            if (pDlgInfo == NULL)
                return FALSE;

            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    if  (
                        pDlgInfo->bDataChanged
                        &&
                        FAILED(ApplyDlgChanges(hwndDlg, pDlgInfo))
                        )
                    {
                        break;
                    }

                    if (pDlgInfo->bScheduleChanged)
                        pDlgInfo->dwFlags |= SSUI_SCHEDULECHANGED;

                    // FALL THROUGH!!!
                }

                case IDCANCEL:
                {
                    EVAL(SUCCEEDED(SchedGroupComboBox_Clear(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_LIST))));
                    EndDialog(hwndDlg, wParam);
                    break;
                }

                case IDC_CUSTOM_GROUP_EDIT:
                {
                    switch (HIWORD(wParam))
                    {
                        case EN_CHANGE:
                        {
                            pDlgInfo->bDataChanged = TRUE;
                            pDlgInfo->bScheduleNameChanged = TRUE;
                            break;
                        }

                        default:
                            break;
                    }

                    break;
                }

                case IDC_CUSTOM_GROUP_LIST:
                {
                    switch (HIWORD(wParam))
                    {
                        case CBN_EDITCHANGE:
                        {
                            pDlgInfo->bDataChanged = TRUE;
                            pDlgInfo->bScheduleNameChanged = TRUE;
                            break;
                        }

                        case CBN_SELCHANGE:
                        {
                            HandleGroupChange(hwndDlg, pDlgInfo);
                            break;
                        }

                        default:
                            break;
                    }

                    break;
                }

                case IDC_CUSTOM_MONTHLY_PERIODIC_LIST1:
                case IDC_CUSTOM_MONTHLY_PERIODIC_LIST2:
                {
                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                        {
                            OnDataChanged(hwndDlg, pDlgInfo);
                            break;
                        }

                        default:
                            break;
                    }

                    break;
                }

                case IDC_CUSTOM_TIME_REPEATEVERY_EDIT:
                case IDC_CUSTOM_DAILY_EVERYNDAYS_EDIT:
                case IDC_CUSTOM_WEEKLY_REPEATWEEKS_EDIT:
                case IDC_CUSTOM_MONTHLY_DAYOFMONTH_DAY_EDIT:
                case IDC_CUSTOM_MONTHLY_DAYOFMONTH_MONTH_EDIT:
                case IDC_CUSTOM_MONTHLY_PERIODIC_EDIT:
                {
                    switch (HIWORD(wParam))
                    {
                        case EN_CHANGE:
                        {
                            OnDataChanged(hwndDlg, pDlgInfo);
                            break;
                        }

                        case EN_KILLFOCUS:
                        {
                            RestrictDlgItemRange(hwndDlg, LOWORD(wParam), pDlgInfo);
                            break;
                        }

                        default:
                            break;
                    }

                    break;
                }

                default:
                {
                    if (HandleDlgButtonClick(hwndDlg, wParam, pDlgInfo))
                        OnDataChanged(hwndDlg, pDlgInfo);

                    break;
                }
            }

            break;
        }

        case WM_NOTIFY:
        {
            switch (wParam)
            {
                case IDC_CUSTOM_TIME_UPDATEAT_TIME:
                case IDC_CUSTOM_TIME_REPEATBETWEEN_START_TIME:
                case IDC_CUSTOM_TIME_REPEATBETWEEN_END_TIME:
                {
                    switch (((NMHDR *)lParam)->code)
                    {
                        case DTN_DATETIMECHANGE:
                        {
                            OnDataChanged(hwndDlg, pDlgInfo);
                            RestrictTimeRepeatEvery(hwndDlg);
                            break;
                        }

                        default:
                            break;
                    }

                    break;
                }

                default:
                    break;
            }

            break;
        }

        default:
            return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// HandleDlgButtonClick
/////////////////////////////////////////////////////////////////////////////
BOOL HandleDlgButtonClick
(
    HWND            hwndDlg,
    WPARAM          wParam,
    SSUIDLGINFO *   pDlgInfo
)
{
    switch (LOWORD(wParam))
    {
        case IDC_CUSTOM_NEWGROUP:
        {
            ASSERT(pDlgInfo != NULL);

            int iReply = IDNO;

            if (pDlgInfo->bDataChanged)
            {
                // Ask user if they want to save changes.
                iReply = SGMessageBox(  hwndDlg,
                                        IDS_CUSTOM_INFO_SAVECHANGES,
                                        MB_YESNOCANCEL | MB_ICONQUESTION);

            }

            switch (iReply)
            {
                case IDYES:
                {
                    // Apply changes
                    if (FAILED(ApplyDlgChanges(hwndDlg, pDlgInfo)))
                        break;

                    // FALL THROUGH!!!
                }

                case IDNO:
                {
                    pDlgInfo->dwFlags |= SSUI_CREATENEWSCHEDULE;
                    *(pDlgInfo->pGroupCookie) = GUID_NULL;

                    SetTaskTriggerToDefaults(&pDlgInfo->ttTaskTrigger);

                    InitDlgInfo(hwndDlg, pDlgInfo);
                    break;
                }

                case IDCANCEL:
                    break;
            }

            break;
        }

        case IDC_CUSTOM_REMOVEGROUP:
        {
            // REVIEW: Can we check to see if the group is really in use. [jaym]

            // Tell user that they are removing a group that may be in use.
            if (SGMessageBox(   hwndDlg,
                                IDS_CUSTOM_INFO_REMOVEGROUP,
                                MB_YESNO | MB_ICONWARNING) == IDYES)
            {
                NOTIFICATIONCOOKIE  groupCookie;
                HWND                hwndCombo = GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_LIST);

                if  (
                    SUCCEEDED(SchedGroupComboBox_GetCurGroup(   hwndCombo,
                                                                &groupCookie))
                    &&
                    SUCCEEDED(DeleteScheduleGroup(&groupCookie))
                    )
                {
                    // Reset the list and select the first group
                    EVAL(SUCCEEDED(SchedGroupComboBox_Fill(hwndCombo)));
                    ComboBox_SetCurSel(hwndCombo, 0);

                    pDlgInfo->dwFlags |= SSUI_SCHEDULEREMOVED;

                    // Make sure we don't ask the user to save changes.
                    pDlgInfo->bDataChanged = FALSE;

                    HandleGroupChange(hwndDlg, pDlgInfo);
                }
                else
                {
                    SGMessageBox(   hwndDlg,
                                    IDS_CUSTOM_ERROR_REMOVEGROUP,
                                    MB_OK | MB_ICONWARNING);
                    break;
                }
            }

            break;
        }

        case IDC_CUSTOM_WEEKLY_DAY1:
        case IDC_CUSTOM_WEEKLY_DAY2:
        case IDC_CUSTOM_WEEKLY_DAY3:
        case IDC_CUSTOM_WEEKLY_DAY4:
        case IDC_CUSTOM_WEEKLY_DAY5:
        case IDC_CUSTOM_WEEKLY_DAY6:
        case IDC_CUSTOM_WEEKLY_DAY7:
        case IDC_CUSTOM_MINIMIZENETUSE:
        {
            return TRUE;
        }

        default:
            return HandleButtonClick(hwndDlg, wParam);
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// HandleButtonClick
/////////////////////////////////////////////////////////////////////////////
BOOL HandleButtonClick
(
    HWND    hwndDlg,
    WPARAM  wParam
)
{
    BOOL bEnable = (IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == BST_CHECKED);

    switch (LOWORD(wParam))
    {
        case IDC_CUSTOM_DAILY:
        case IDC_CUSTOM_WEEKLY:
        case IDC_CUSTOM_MONTHLY:
        {
            SetDlgCustomType(hwndDlg, LOWORD(wParam));
            break;
        }

        case IDC_CUSTOM_DAILY_EVERYNDAYS:
        {
            Edit_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_DAILY_EVERYNDAYS_EDIT), bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_DAILY_EVERYNDAYS_SPIN), bEnable);
            break;
        }

        case IDC_CUSTOM_DAILY_EVERYWEEKDAY:
        {
            Edit_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_DAILY_EVERYNDAYS_EDIT), !bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_DAILY_EVERYNDAYS_SPIN), !bEnable);
            break;
        }

        case IDC_CUSTOM_MONTHLY_DAYOFMONTH:
        {
            Edit_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH_DAY_EDIT), bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH_DAY_SPIN), bEnable);
            Edit_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH_MONTH_EDIT), bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH_MONTH_SPIN), bEnable);

            ListBox_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_LIST1), !bEnable);
            ListBox_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_LIST2), !bEnable);
            Edit_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_EDIT), !bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_SPIN), !bEnable);
            break;
        }

        case IDC_CUSTOM_MONTHLY_PERIODIC:
        {
            ListBox_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_LIST1), bEnable);
            ListBox_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_LIST2), bEnable);
            Edit_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_EDIT), bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_SPIN), bEnable);

            Edit_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH_DAY_EDIT), !bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH_DAY_SPIN), !bEnable);
            Edit_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH_MONTH_EDIT), !bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH_MONTH_SPIN), !bEnable);
            break;
        }

        case IDC_CUSTOM_TIME_UPDATEAT:
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_UPDATEAT_TIME), bEnable);
            Edit_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATEVERY_EDIT), !bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATEVERY_SPIN), !bEnable);

            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN), !bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN_START_TIME), !bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN_END_TIME), !bEnable);
            break;
        }

        case IDC_CUSTOM_TIME_REPEATEVERY:
        {
            Edit_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATEVERY_EDIT), bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATEVERY_SPIN), bEnable);

            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN), bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_UPDATEAT_TIME), !bEnable);

            bEnable = (IsDlgButtonChecked(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN) == BST_CHECKED);

            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN_START_TIME), bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN_END_TIME), bEnable);

            break;
        }

        case IDC_CUSTOM_TIME_REPEATBETWEEN:
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN_START_TIME), bEnable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN_END_TIME), bEnable);

            // Restrict 'repeat between' to time range - 1hr
            if (bEnable)
                RestrictTimeRepeatEvery(hwndDlg);
            else
            {
                UpDown_SetRange(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATEVERY_SPIN),
                                DEFAULT_REPEATUPDATE_HRS_MIN, DEFAULT_REPEATUPDATE_HRS_MAX);
            }

            break;
        }

        default:
            return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// HandleGroupChange
/////////////////////////////////////////////////////////////////////////////
BOOL HandleGroupChange
(
    HWND            hwndDlg,
    SSUIDLGINFO *   pDlgInfo
)
{
    int     iReply = IDNO;
    BOOL    bChanged = FALSE;

    if (pDlgInfo->bDataChanged)
    {
        // Ask user if they want to save changes.
        iReply = SGMessageBox(  hwndDlg,
                                IDS_CUSTOM_INFO_SAVECHANGES,
                                MB_YESNO | MB_ICONQUESTION);
    }

    switch (iReply)
    {
        case IDYES:
        {
            // Apply changes
            if (FAILED(ApplyDlgChanges(hwndDlg, pDlgInfo)))
                break;

            // FALL THROUGH!!!
        }

        case IDNO:
        {
            // No longer creating new schedule, the
            // user has selected an existing schedule.
            pDlgInfo->dwFlags &= ~SSUI_CREATENEWSCHEDULE;

            EVAL(SUCCEEDED(SchedGroupComboBox_GetCurGroup(  GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_LIST),
                                                            pDlgInfo->pGroupCookie)));

            EVAL(SUCCEEDED(GetSchedGroupTaskTrigger( pDlgInfo->pGroupCookie,
                                                     &pDlgInfo->ttTaskTrigger)));

            InitDlgInfo(hwndDlg, pDlgInfo);

            pDlgInfo->bScheduleChanged = TRUE;
            bChanged = TRUE;

            break;
        }

        default:
        {
            ASSERT(FALSE);
            break;
        }
    }

    return bChanged;
}

/////////////////////////////////////////////////////////////////////////////
// InitDlgCtls
/////////////////////////////////////////////////////////////////////////////
void InitDlgCtls
(
    HWND            hwndDlg,
    SSUIDLGINFO *   pDlgInfo
)
{
    DWORD i;

    // Reposition the dialog controls.
    PositionDlgCtls(hwndDlg);

    // Load the common combo box values.
    for (i = 0; i < ARRAYSIZE(c_rgnCtlItems); i++)
    {
        HWND hwndCtl = GetDlgItem(hwndDlg, c_rgnCtlItems[i].idContainer);
        
        ASSERT(hwndCtl != NULL);

        for (int j = c_rgnCtlItems[i].idFirst; j <= c_rgnCtlItems[i].idLast; j++)
        {
            TCHAR szString[MAX_LOADSTRING_LEN];
            EVAL(MLLoadString(j, szString, MAX_LOADSTRING_LEN) != 0);
            EVAL(ComboBox_AddString(hwndCtl, szString) != CB_ERR);
        }
    }

    DWORD dwFirstDay = 0;
    EVAL(GetLocaleInfo( LOCALE_USER_DEFAULT,
                        LOCALE_IFIRSTDAYOFWEEK,
                        (char *)&dwFirstDay,
                        SIZEOF(DWORD)));
    dwFirstDay = LOCALE_SDAYNAME1 + (dwFirstDay - TEXT('0'));

    // Load check and combo boxes with the days of the weeks
    int     iDay;
    HWND    hwndCombo = GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_LIST2);
    TCHAR   szDayName[32];

    for (i = dwFirstDay; i < dwFirstDay+7; i++)
    {
        iDay = ((i <= LOCALE_SDAYNAME7) ? i : (LOCALE_SDAYNAME1 + (i - LOCALE_SDAYNAME7 - 1)));

        EVAL(GetLocaleInfo( LOCALE_USER_DEFAULT,
                            iDay,
                            szDayName,
                            ARRAYSIZE(szDayName)));

        SetDlgItemText( hwndDlg,
                        IDC_CUSTOM_WEEKLY_DAY1 + (i - dwFirstDay),
                        szDayName);

        int iIndex = ComboBox_AddString(hwndCombo, szDayName);

        // Initialize the lookup table for the combobox as we go.
        if (iDay == LOCALE_SDAYNAME1)
            s_rgwDaysOfTheWeek[iIndex] = TASK_MONDAY;
        else if (iDay == LOCALE_SDAYNAME2)
            s_rgwDaysOfTheWeek[iIndex] = TASK_TUESDAY;
        else if (iDay == LOCALE_SDAYNAME3)
            s_rgwDaysOfTheWeek[iIndex] = TASK_WEDNESDAY;
        else if (iDay == LOCALE_SDAYNAME4)
            s_rgwDaysOfTheWeek[iIndex] = TASK_THURSDAY;
        else if (iDay == LOCALE_SDAYNAME5)
            s_rgwDaysOfTheWeek[iIndex] = TASK_FRIDAY;
        else if (iDay == LOCALE_SDAYNAME6)
            s_rgwDaysOfTheWeek[iIndex] = TASK_SATURDAY;
        else if (iDay == LOCALE_SDAYNAME7)
            s_rgwDaysOfTheWeek[iIndex] = TASK_SUNDAY;
        else    
            ASSERT(FALSE);
   }

    // Show/Hide controls based on dialog type.
    if (pDlgInfo->dwFlags & SSUI_CREATENEWSCHEDULE)
    {
        // Hide the 'New' and 'Remove' buttons if
        // we are creating a new schedule group.
        ShowWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_NEWGROUP), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_REMOVEGROUP), SW_HIDE);

        // Hide the dropdown schedule group list.
        ShowWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_LIST), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_EDIT), SW_SHOW);
    }
    else if (pDlgInfo->dwFlags & SSUI_EDITSCHEDULE)
    {
        // Hide the schedule group name edit.
        ShowWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_EDIT), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_LIST), SW_SHOW);

        EVAL(SUCCEEDED(SchedGroupComboBox_Fill(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_LIST))));
        EVAL(SUCCEEDED(SchedGroupComboBox_SetCurGroup(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_LIST), pDlgInfo->pGroupCookie)));
    }
    else
        ASSERT(FALSE);

    // Setup spinner ranges.
    UpDown_SetRange(GetDlgItem(hwndDlg, IDC_CUSTOM_DAILY_EVERYNDAYS_SPIN),
                    DEFAULT_DAILY_EVERYNDAYS_MIN, DEFAULT_DAILY_EVERYNDAYS_MAX);
    UpDown_SetRange(GetDlgItem(hwndDlg, IDC_CUSTOM_WEEKLY_REPEATWEEKS_SPIN),
                    DEFAULT_WEEKLY_REPEATWEEKS_MIN, DEFAULT_WEEKLY_REPEATWEEKS_MAX);
    UpDown_SetRange(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH_DAY_SPIN),
                    DEFAULT_MONTHLY_DAY_MIN, DEFAULT_MONTHLY_DAY_MAX);
    UpDown_SetRange(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH_MONTH_SPIN),
                    DEFAULT_MONTHLY_MONTHS_MIN, DEFAULT_MONTHLY_MONTHS_MAX);
    UpDown_SetRange(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_SPIN),
                    DEFAULT_MONTHLY_MONTHS_MIN, DEFAULT_MONTHLY_MONTHS_MAX);
    UpDown_SetRange(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATEVERY_SPIN),
                    DEFAULT_REPEATUPDATE_HRS_MIN, DEFAULT_REPEATUPDATE_HRS_MAX);

    // Setup the time picker controls to use a short time format with no seconds.
    TCHAR szTimeFormat[MAX_SHORTTIMEFORMAT_LEN];
    LPTSTR pszTimeFormat = szTimeFormat;
    EVAL(GetLocaleInfo( LOCALE_USER_DEFAULT,
                        LOCALE_STIMEFORMAT,
                        szTimeFormat,
                        ARRAYSIZE(szTimeFormat)));

    TCHAR szTimeSep[MAX_TIMESEP_LEN];
    EVAL(GetLocaleInfo( LOCALE_USER_DEFAULT,
                        LOCALE_STIME,
                        szTimeSep,
                        ARRAYSIZE(szTimeSep)));
    int cchTimeSep = lstrlen(szTimeSep);

    TCHAR szShortTimeFormat[MAX_SHORTTIMEFORMAT_LEN];
    LPTSTR pszShortTimeFormat = szShortTimeFormat;

    // Remove the seconds format string and preceeding separator.
    while (*pszTimeFormat)
    {
        if ((*pszTimeFormat != TEXT('s')) && (*pszTimeFormat != TEXT('S')))
            *pszShortTimeFormat++ = *pszTimeFormat;
        else
        {
            *pszShortTimeFormat = TEXT('\0');

            StrTrim(szShortTimeFormat, TEXT(" "));
            StrTrim(szShortTimeFormat, szTimeSep);

            pszShortTimeFormat = szShortTimeFormat + lstrlen(szShortTimeFormat);
        }

        pszTimeFormat++;
    }

    *pszShortTimeFormat = TEXT('\0');

    // Set the format for the time picker controls.
    DateTime_SetFormat(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_UPDATEAT_TIME), szShortTimeFormat);
    DateTime_SetFormat(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN_START_TIME), szShortTimeFormat);
    DateTime_SetFormat(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN_END_TIME), szShortTimeFormat);
}

/////////////////////////////////////////////////////////////////////////////
// PositionDlgCtls
/////////////////////////////////////////////////////////////////////////////
void PositionDlgCtls
(
    HWND hwndDlg
)
{
    RECT rectDestGroup;
    GetWindowRect(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_DEST), &rectDestGroup);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (POINT *)&rectDestGroup, 2);

    for (int i = 0; i < ARRAYSIZE(c_rgnCtlGroups); i++)
    {
        HWND hwndGroup = GetDlgItem(hwndDlg, c_rgnCtlGroups[i].idContainer);
        
        ASSERT(hwndGroup != NULL);

        RECT rectSrcGroup;
        GetWindowRect(hwndGroup, &rectSrcGroup);
        MapWindowPoints(HWND_DESKTOP, hwndDlg, (POINT *)&rectSrcGroup, 2);

        for (int j = c_rgnCtlGroups[i].idFirst; j <= c_rgnCtlGroups[i].idLast; j++)
        {
            HWND hwndItem = GetDlgItem(hwndDlg, j);

            RECT rectItem;
            GetWindowRect(hwndItem, &rectItem);
            MapWindowPoints(HWND_DESKTOP, hwndDlg, (POINT *)&rectItem, 2);

            MoveWindow( hwndItem,
                        rectDestGroup.left + (rectItem.left - rectSrcGroup.left),
                        rectDestGroup.top + (rectItem.top - rectSrcGroup.top),
                        (rectItem.right - rectItem.left),
                        (rectItem.bottom - rectItem.top),
                        FALSE);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// InitDlgDefaults
/////////////////////////////////////////////////////////////////////////////
void InitDlgDefaults
(
    HWND            hwndDlg,
    SSUIDLGINFO *   pDlgInfo
)
{
    TCHAR szDefaultName[MAX_GROUPNAME_LEN];

    // Init control defaults for new schedule group.
    MLLoadString(IDS_CUSTOM_NEWSCHEDDEFNAME, szDefaultName, ARRAYSIZE(szDefaultName));

    if (pDlgInfo->dwFlags & SSUI_CREATENEWSCHEDULE)
    {
        SetDlgItemText(hwndDlg, IDC_CUSTOM_GROUP_EDIT, szDefaultName);
        SetDlgItemText(hwndDlg, IDC_CUSTOM_GROUP_LIST, szDefaultName);
        pDlgInfo->bDataChanged = FALSE;
    }

    // If we are creating a new schedule, everything needs to be saved.
    pDlgInfo->bDataChanged = ((pDlgInfo->dwFlags & SSUI_CREATENEWSCHEDULE) ? TRUE : FALSE);

    // Daily group
    CheckRadioButton(hwndDlg, IDC_TYPE_GROUP_FIRST, IDC_TYPE_GROUP_LAST, IDC_CUSTOM_DAILY);

    // Daily group properties
    CheckRadioButton(hwndDlg, IDC_DAILY_GROUP_FIRST, IDC_DAILY_GROUP_LAST, IDC_CUSTOM_DAILY_EVERYNDAYS);
    HandleDlgButtonClick(hwndDlg, IDC_CUSTOM_DAILY_EVERYNDAYS, NULL);

    SetDlgItemInt(hwndDlg, IDC_CUSTOM_DAILY_EVERYNDAYS_EDIT, DEFAULT_DAILY_EVERYNDAYS, FALSE);

    // Weekly group properties
    SetDlgItemInt(hwndDlg, IDC_CUSTOM_WEEKLY_REPEATWEEKS_EDIT, DEFAULT_WEEKLY_REPEATWEEKS, FALSE);

    SYSTEMTIME stLocal;
    GetLocalTime(&stLocal);

    WORD rgfDaysOfTheWeek = (1 << stLocal.wDayOfWeek);
    SetDlgWeekDays(hwndDlg, rgfDaysOfTheWeek);

    // Monthly group properties
    CheckRadioButton(hwndDlg, IDC_MONTHLY_GROUP_FIRST, IDC_MONTHLY_GROUP_LAST, IDC_CUSTOM_MONTHLY_DAYOFMONTH);
    HandleDlgButtonClick(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH, NULL);

    SetDlgItemInt(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH_DAY_EDIT, stLocal.wDay, FALSE);
    SetDlgItemInt(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH_MONTH_EDIT, DEFAULT_MONTHLY_MONTHS, FALSE);

    // REVIEW: Create code to figure out item to select for current date. [jaym]
    ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_LIST1), 0);
    ComboBox_SetCurSel( GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_LIST2),
                        DayIndexFromDaysOfTheWeekFlags(rgfDaysOfTheWeek));

    SetDlgItemInt(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_EDIT, DEFAULT_MONTHLY_MONTHS, FALSE);

    // Time properties
    CheckRadioButton(hwndDlg, IDC_TIME_GROUP_FIRST, IDC_TIME_GROUP_LAST, IDC_CUSTOM_TIME_UPDATEAT);
    HandleDlgButtonClick(hwndDlg, IDC_CUSTOM_TIME_UPDATEAT, NULL);

    SYSTEMTIME st = { 0 };
    GetLocalTime(&st);
    st.wHour    = DEFAULT_UPDATETIME_HRS;
    st.wMinute  = DEFAULT_UPDATETIME_MINS;
    DateTime_SetSystemtime(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_UPDATEAT_TIME), GDT_VALID, &st);

    SetDlgItemInt(hwndDlg, IDC_CUSTOM_TIME_REPEATEVERY_EDIT, DEFAULT_REPEATUPDATE_HRS, FALSE);

    st.wHour    = DEFAULT_REPEAT_START_HRS;
    st.wMinute  = DEFAULT_REPEAT_START_MINS;
    DateTime_SetSystemtime(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN_START_TIME), GDT_VALID, &st);

    st.wHour    = DEFAULT_REPEAT_END_HRS;
    st.wMinute  = DEFAULT_REPEAT_END_MINS;
    DateTime_SetSystemtime(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN_END_TIME), GDT_VALID, &st);

    // Disable some controls if we are a predefined group.
    HWND hwndGroupList = GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_LIST);
    HWND hwndGroupEdit = FindWindowEx(  hwndGroupList,
                                        NULL,
                                        TEXT("Edit"),
                                        NULL);

    if (*(pDlgInfo->pGroupCookie) == NOTFCOOKIE_SCHEDULE_GROUP_DAILY)
    {
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_DAILY), TRUE);
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_WEEKLY), FALSE);
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY), FALSE);
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_REMOVEGROUP), FALSE);
        Edit_SetReadOnly(hwndGroupEdit, TRUE);
    }
    else if (*(pDlgInfo->pGroupCookie) == NOTFCOOKIE_SCHEDULE_GROUP_WEEKLY)
    {
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_DAILY), FALSE);
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_WEEKLY), TRUE);
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY), FALSE);
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_REMOVEGROUP), FALSE);
        Edit_SetReadOnly(hwndGroupEdit, TRUE);
    }
    else if (*(pDlgInfo->pGroupCookie) == NOTFCOOKIE_SCHEDULE_GROUP_MONTHLY)
    {
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_DAILY), FALSE);
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_WEEKLY), FALSE);
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY), TRUE);
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_REMOVEGROUP), FALSE);
        Edit_SetReadOnly(hwndGroupEdit, TRUE);
    }
    else
    {
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_DAILY), TRUE);
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_WEEKLY), TRUE);
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY), TRUE);
        Button_Enable(GetDlgItem(hwndDlg, IDC_CUSTOM_REMOVEGROUP), TRUE);
        Edit_SetReadOnly(hwndGroupEdit, FALSE);
    }

    // Make sure the combo redraws so the enable
    // or disable of the edit control looks good.
    RedrawWindow(hwndGroupList, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

    // Vary start of next update
    CheckDlgButton(hwndDlg, IDC_CUSTOM_MINIMIZENETUSE, BST_CHECKED);
}

/////////////////////////////////////////////////////////////////////////////
// SetDlgCustomType
/////////////////////////////////////////////////////////////////////////////
void SetDlgCustomType
(
    HWND    hwndDlg,
    int     iType
)
{
    int i;

    for (i = IDC_CUSTOM_DAILY_FIRST; i <= IDC_CUSTOM_DAILY_LAST; i++)
        ShowWindow(GetDlgItem(hwndDlg, i), ((iType == IDC_CUSTOM_DAILY) ? SW_SHOW : SW_HIDE));

    for (i = IDC_CUSTOM_WEEKLY_FIRST; i <= IDC_CUSTOM_WEEKLY_LAST; i++)
        ShowWindow(GetDlgItem(hwndDlg, i), ((iType == IDC_CUSTOM_WEEKLY) ? SW_SHOW : SW_HIDE));

    for (i = IDC_CUSTOM_MONTHLY_FIRST; i <= IDC_CUSTOM_MONTHLY_LAST; i++)
        ShowWindow(GetDlgItem(hwndDlg, i), ((iType == IDC_CUSTOM_MONTHLY) ? SW_SHOW : SW_HIDE));
}

/////////////////////////////////////////////////////////////////////////////
// InitDlgInfo
/////////////////////////////////////////////////////////////////////////////
void InitDlgInfo
(
    HWND            hwndDlg,
    SSUIDLGINFO *   pDlgInfo
)
{
    int             nIDTriggerType;
    TASK_TRIGGER *  pTT = &(pDlgInfo->ttTaskTrigger);

#ifdef DEBUG
    DumpTaskTrigger(pTT);
#endif  // DEBUG

    pDlgInfo->bInitializing = TRUE;

    // Initialize defaults for dialog controls.
    InitDlgDefaults(hwndDlg, pDlgInfo);

    CheckDlgButton( hwndDlg,
                    IDC_CUSTOM_MINIMIZENETUSE,
                    ((pTT->wRandomMinutesInterval == 0) ? BST_UNCHECKED : BST_CHECKED));

    // Initialize the 'Time' settings.
    if (pTT->MinutesInterval == 0)
    {
        // Update at N
        CheckRadioButton(hwndDlg, IDC_TIME_GROUP_FIRST, IDC_TIME_GROUP_LAST, IDC_CUSTOM_TIME_UPDATEAT);
        HandleDlgButtonClick(hwndDlg, IDC_CUSTOM_TIME_UPDATEAT, NULL);

        SYSTEMTIME st = { 0 };
        GetLocalTime(&st);
        st.wHour    = pTT->wStartHour;
        st.wMinute  = pTT->wStartMinute;
        DateTime_SetSystemtime(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_UPDATEAT_TIME), GDT_VALID, &st);
    }
    else
    {
        // Repeat every N hour(s)
        CheckRadioButton(hwndDlg, IDC_TIME_GROUP_FIRST, IDC_TIME_GROUP_LAST, IDC_CUSTOM_TIME_REPEATEVERY);
        HandleDlgButtonClick(hwndDlg, IDC_CUSTOM_TIME_REPEATEVERY, NULL);

        SetDlgItemInt(  hwndDlg,
                        IDC_CUSTOM_TIME_REPEATEVERY_EDIT,
                        ((pDlgInfo->dwRepeatHrsAreMins)
                            ? pTT->MinutesInterval
                            : (pTT->MinutesInterval / 60)),
                        FALSE);

        // Between X and Y
        if (pTT->MinutesDuration != (24 * 60))
        {
            CheckDlgButton(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN, BST_CHECKED);
            HandleDlgButtonClick(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN, NULL);

            SYSTEMTIME st = { 0 };
            GetLocalTime(&st);
            st.wHour    = pTT->wStartHour;
            st.wMinute  = pTT->wStartMinute;
            DateTime_SetSystemtime(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN_START_TIME), GDT_VALID, &st);

            DWORD dwEndTime = IncTime(  MAKELONG(pTT->wStartHour, pTT->wStartMinute),
                                        (pTT->MinutesDuration / 60),
                                        (pTT->MinutesDuration % 60));

            st.wHour = LOWORD(dwEndTime);
            st.wMinute = HIWORD(dwEndTime);
            DateTime_SetSystemtime(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN_END_TIME), GDT_VALID, &st);
        }
    }

    // Initialize the 'Days' settings.
    switch (pTT->TriggerType)
    {
        case TASK_TIME_TRIGGER_DAILY:
        {
            nIDTriggerType = IDC_CUSTOM_DAILY;

            // Every N day(s)
            CheckRadioButton(hwndDlg, IDC_DAILY_GROUP_FIRST, IDC_DAILY_GROUP_LAST, IDC_CUSTOM_DAILY_EVERYNDAYS);
            HandleDlgButtonClick(hwndDlg, IDC_CUSTOM_DAILY_EVERYNDAYS, NULL);

            SetDlgItemInt(hwndDlg, IDC_CUSTOM_DAILY_EVERYNDAYS_EDIT, pTT->Type.Daily.DaysInterval, FALSE);
            break;
        }

        case TASK_TIME_TRIGGER_WEEKLY:
        {
            // Special case for Weekly repeating every 1 week
            if  (
                (pTT->Type.Weekly.WeeksInterval == 1)
                &&
                (pTT->Type.Weekly.rgfDaysOfTheWeek == TASK_WEEKDAYS)
                )
            {
                if (*(pDlgInfo->pGroupCookie) == NOTFCOOKIE_SCHEDULE_GROUP_WEEKLY)
                {
                    nIDTriggerType = IDC_CUSTOM_WEEKLY;

                    // Weekly -- Every 1 week on MTWTF
                    SetDlgItemInt(hwndDlg, IDC_CUSTOM_WEEKLY_REPEATWEEKS_EDIT, 1, FALSE);
                    SetDlgWeekDays(hwndDlg, TASK_WEEKDAYS);
                }
                else
                {
                    nIDTriggerType = IDC_CUSTOM_DAILY;

                    // Daily -- Every weekday
                    CheckRadioButton(hwndDlg, IDC_DAILY_GROUP_FIRST, IDC_DAILY_GROUP_LAST, IDC_CUSTOM_DAILY_EVERYWEEKDAY);
                    HandleDlgButtonClick(hwndDlg, IDC_CUSTOM_DAILY_EVERYWEEKDAY, NULL);
                }
            }
            else
            {
                nIDTriggerType = IDC_CUSTOM_WEEKLY;

                // Weekly -- Every N weeks on SMTWTFS
                 
                // Every N weeks
                SetDlgItemInt(hwndDlg, IDC_CUSTOM_WEEKLY_REPEATWEEKS_EDIT, pTT->Type.Weekly.WeeksInterval, FALSE);

                // on SMTWTFS
                SetDlgWeekDays(hwndDlg, pTT->Type.Weekly.rgfDaysOfTheWeek);
            }
            break;
        }

        case TASK_TIME_TRIGGER_MONTHLYDATE:
        {
            nIDTriggerType = IDC_CUSTOM_MONTHLY;

            // Monthly -- Day X of every Y month(s)
            CheckRadioButton(hwndDlg, IDC_MONTHLY_GROUP_FIRST, IDC_MONTHLY_GROUP_LAST, IDC_CUSTOM_MONTHLY_DAYOFMONTH);
            HandleDlgButtonClick(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH, NULL);

            // Day X
            SetDlgItemInt(  hwndDlg,
                            IDC_CUSTOM_MONTHLY_DAYOFMONTH_DAY_EDIT,
                            DayFromDaysFlags(pTT->Type.MonthlyDate.rgfDays),
                            FALSE);

            // of every Y month(s)
            SetDlgItemMonth(hwndDlg,
                            IDC_CUSTOM_MONTHLY_DAYOFMONTH_MONTH_EDIT,
                            pTT->Type.MonthlyDate.rgfMonths);
            break;
        }

        case TASK_TIME_TRIGGER_MONTHLYDOW:
        {
            nIDTriggerType = IDC_CUSTOM_MONTHLY;

            // Monthly -- The X Y of every Z month(s)
            CheckRadioButton(hwndDlg, IDC_MONTHLY_GROUP_FIRST, IDC_MONTHLY_GROUP_LAST, IDC_CUSTOM_MONTHLY_PERIODIC);
            HandleDlgButtonClick(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC, NULL);

            // The X
            ComboBox_SetCurSel( GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_LIST1),
                                pTT->Type.MonthlyDOW.wWhichWeek-1);

            // Y
            ComboBox_SetCurSel( GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_LIST2),
                                DayIndexFromDaysOfTheWeekFlags(pTT->Type.MonthlyDOW.rgfDaysOfTheWeek));

            // of every Z month(s)
            SetDlgItemMonth(hwndDlg,
                            IDC_CUSTOM_MONTHLY_PERIODIC_EDIT,
                            pTT->Type.MonthlyDOW.rgfMonths);
            break;
        }

        default:
        {
            ASSERT(FALSE);
            break;
        }
    }

    CheckRadioButton(hwndDlg, IDC_TYPE_GROUP_FIRST, IDC_TYPE_GROUP_LAST, nIDTriggerType);
    HandleDlgButtonClick(hwndDlg, nIDTriggerType, NULL);

    SetTaskTriggerFromDlg(hwndDlg, pDlgInfo);
    SetDlgItemNextUpdate(hwndDlg, IDC_CUSTOM_NEXTUPDATE, pDlgInfo);

    // Set focus to the group name control.
    if (IsWindowVisible(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_EDIT)))
        SetFocus(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_EDIT));
    else
        SetFocus(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_LIST));

    // Make sure we get asked to save for new groups.
    pDlgInfo->bDataChanged = ((pDlgInfo->dwFlags & SSUI_CREATENEWSCHEDULE) ? TRUE : FALSE);

    pDlgInfo->bScheduleNameChanged = FALSE;

    pDlgInfo->bInitializing = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// SetDlgWeekDays
/////////////////////////////////////////////////////////////////////////////
void SetDlgWeekDays
(
    HWND hwndDlg,
    WORD rgfDaysOfTheWeek
)
{
    // If it doesn't have any days, stick one in arbitrarily.
    if (rgfDaysOfTheWeek == 0)
        rgfDaysOfTheWeek = TASK_SUNDAY;

    DWORD dwFirstDay = 0;
    EVAL(GetLocaleInfo( LOCALE_USER_DEFAULT,
                        LOCALE_IFIRSTDAYOFWEEK,
                        (char *)&dwFirstDay,
                        SIZEOF(DWORD)));

    dwFirstDay = LOCALE_SDAYNAME1 + (dwFirstDay - TEXT('0'));

    for (DWORD i = dwFirstDay; i <= dwFirstDay+7; i++)
    {
        BOOL    bCheck; 
        int     iDay = ((i <= LOCALE_SDAYNAME7) ? i
                                                : (LOCALE_SDAYNAME1 + (i - LOCALE_SDAYNAME7 - 1)));

        if (iDay == LOCALE_SDAYNAME1)
            bCheck = (rgfDaysOfTheWeek & TASK_MONDAY);
        else if (iDay == LOCALE_SDAYNAME2)
            bCheck = (rgfDaysOfTheWeek & TASK_TUESDAY);
        else if (iDay == LOCALE_SDAYNAME3)
            bCheck = (rgfDaysOfTheWeek & TASK_WEDNESDAY);
        else if (iDay == LOCALE_SDAYNAME4)
            bCheck = (rgfDaysOfTheWeek & TASK_THURSDAY);
        else if (iDay == LOCALE_SDAYNAME5)
            bCheck = (rgfDaysOfTheWeek & TASK_FRIDAY);
        else if (iDay == LOCALE_SDAYNAME6)
            bCheck = (rgfDaysOfTheWeek & TASK_SATURDAY);
        else if (iDay == LOCALE_SDAYNAME7)
            bCheck = (rgfDaysOfTheWeek & TASK_SUNDAY);
        else
            ASSERT(FALSE);

        CheckDlgButton( hwndDlg,
                        IDC_CUSTOM_WEEKLY_DAY1 + (i - dwFirstDay),
                        (bCheck ? BST_CHECKED : BST_UNCHECKED));
    }
}

/////////////////////////////////////////////////////////////////////////////
// GetDlgWeekDays
/////////////////////////////////////////////////////////////////////////////
WORD GetDlgWeekDays
(
    HWND hwndDlg
)
{
    WORD wDays = 0;

    DWORD dwFirstDay = 0;
    EVAL(GetLocaleInfo( LOCALE_USER_DEFAULT,
                        LOCALE_IFIRSTDAYOFWEEK,
                        (char *)&dwFirstDay,
                        SIZEOF(DWORD)));

    dwFirstDay = LOCALE_SDAYNAME1 + (dwFirstDay - TEXT('0'));

    for (DWORD i = dwFirstDay; i <= dwFirstDay+7; i++)
    {
        if (IsDlgButtonChecked( hwndDlg,
                                IDC_CUSTOM_WEEKLY_DAY1 + (i - dwFirstDay)) == BST_CHECKED)
        {
            int iDay = ((i <= LOCALE_SDAYNAME7) ? i
                                                : (LOCALE_SDAYNAME1 + (i - LOCALE_SDAYNAME7 - 1)));

            
            if (iDay == LOCALE_SDAYNAME1)
                wDays |= TASK_MONDAY;
            else if (iDay == LOCALE_SDAYNAME2)
                wDays |= TASK_TUESDAY;
            else if (iDay == LOCALE_SDAYNAME3)
                wDays |= TASK_WEDNESDAY;
            else if (iDay == LOCALE_SDAYNAME4)
                wDays |= TASK_THURSDAY;
            else if (iDay == LOCALE_SDAYNAME5)
                wDays |= TASK_FRIDAY;
            else if (iDay == LOCALE_SDAYNAME6)
                wDays |= TASK_SATURDAY;
            else if (iDay == LOCALE_SDAYNAME7)
                wDays |= TASK_SUNDAY;
            else
                ASSERT(FALSE);
        }
    }

    return wDays;
}

/////////////////////////////////////////////////////////////////////////////
// SetTaskTriggerFromDlg
/////////////////////////////////////////////////////////////////////////////
void SetTaskTriggerFromDlg
(
    HWND            hwndDlg,
    SSUIDLGINFO *   pDlgInfo
)
{
    BOOL            bTranslated;
    TASK_TRIGGER *  pTT = &pDlgInfo->ttTaskTrigger;

    ZeroMemory(pTT, SIZEOF(TASK_TRIGGER));
    pTT->cbTriggerSize = SIZEOF(TASK_TRIGGER);

    // Init start date to now.
    SYSTEMTIME st;
    GetLocalTime(&st);
    pTT->wBeginYear    = st.wYear;
    pTT->wBeginMonth   = st.wMonth;
    pTT->wBeginDay     = st.wDay;

    // Vary start of update
    if (IsDlgButtonChecked(hwndDlg, IDC_CUSTOM_MINIMIZENETUSE) == BST_CHECKED)
        pTT->wRandomMinutesInterval = DEFAULT_RANDOM_MINUTES_INTERVAL;
    else
        pTT->wRandomMinutesInterval = 0;

    // Initialize the 'Time' related settings.
    if (IsDlgButtonChecked(hwndDlg, IDC_CUSTOM_TIME_UPDATEAT) == BST_CHECKED)
    {
        // Update at N
        SYSTEMTIME st = { 0 };
        DateTime_GetSystemtime(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_UPDATEAT_TIME), &st);

        pTT->wStartHour    = st.wHour;
        pTT->wStartMinute  = st.wMinute;
    }
    else
    {
        // Repeat every N hours
        ASSERT(IsDlgButtonChecked(hwndDlg, IDC_CUSTOM_TIME_REPEATEVERY) == BST_CHECKED)

        // Between X and Y
        if (IsDlgButtonChecked(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN) == BST_CHECKED)
        {
            // Between X
            SYSTEMTIME st = { 0 };
            DateTime_GetSystemtime( GetDlgItem( hwndDlg, 
                                                IDC_CUSTOM_TIME_REPEATBETWEEN_START_TIME),
                                    &st);

            pTT->wStartHour    = st.wHour;
            pTT->wStartMinute  = st.wMinute;

            WORD wStartMins = (st.wHour * 60) + st.wMinute;

            DateTime_GetSystemtime( GetDlgItem( hwndDlg,
                                                IDC_CUSTOM_TIME_REPEATBETWEEN_END_TIME),
                                    &st);

            WORD wEndMins = (st.wHour * 60) + st.wMinute;

            // And Y
            if (wStartMins > wEndMins)
                pTT->MinutesDuration = (1440 - wStartMins) + wEndMins;
            else
                pTT->MinutesDuration = wEndMins - wStartMins;
        }
        else
        {
            // If we are not between a hour range, begin update at the start of the day.
            pTT->wStartHour         = 0;    // 12:00am
            pTT->wStartMinute       = 0;
            pTT->MinutesDuration    = (24 * 60);
        }

        // Set the interval.
        int iHours = GetDlgItemInt( hwndDlg,
                                    IDC_CUSTOM_TIME_REPEATEVERY_EDIT,
                                    &bTranslated,
                                    FALSE);

        ASSERT(bTranslated && (iHours > 0));

        if (pDlgInfo->dwRepeatHrsAreMins)
            pTT->MinutesInterval = iHours;
        else
            pTT->MinutesInterval = iHours * 60;
    }

    // Initialize the 'Days' related settings.

    // Daily schedule
    if (IsDlgButtonChecked(hwndDlg, IDC_CUSTOM_DAILY) == BST_CHECKED)
    {
        // Daily -- Every N days
        if (IsDlgButtonChecked(hwndDlg, IDC_CUSTOM_DAILY_EVERYNDAYS) == BST_CHECKED)
        {
            pTT->TriggerType = TASK_TIME_TRIGGER_DAILY;
            pTT->Type.Daily.DaysInterval = GetDlgItemInt(   hwndDlg,
                                                            IDC_CUSTOM_DAILY_EVERYNDAYS_EDIT,
                                                            &bTranslated,
                                                            FALSE);
        }
        else
        {
            // Daily -- Every Weekday
            ASSERT(IsDlgButtonChecked(hwndDlg, IDC_CUSTOM_DAILY_EVERYWEEKDAY));

            pTT->TriggerType = TASK_TIME_TRIGGER_WEEKLY;
            pTT->Type.Weekly.WeeksInterval = 1;
            pTT->Type.Weekly.rgfDaysOfTheWeek = TASK_WEEKDAYS;
                                                        
        }
    }
    // Weekly schedule
    else if (IsDlgButtonChecked(hwndDlg, IDC_CUSTOM_WEEKLY) == BST_CHECKED)
    {
        pTT->TriggerType = TASK_TIME_TRIGGER_WEEKLY;

        // Weekly -- Repeat every N days
        pTT->Type.Weekly.WeeksInterval = GetDlgItemInt( hwndDlg,
                                                        IDC_CUSTOM_WEEKLY_REPEATWEEKS_EDIT,
                                                        &bTranslated,
                                                        FALSE);

        // Weekly -- Days of the week
        pTT->Type.Weekly.rgfDaysOfTheWeek = GetDlgWeekDays(hwndDlg);
    }
    // Monthly schedule
    else
    {
        ASSERT(IsDlgButtonChecked(hwndDlg, IDC_CUSTOM_MONTHLY) == BST_CHECKED);

        // Monthly -- Day X of every Y month(s)
        if (IsDlgButtonChecked(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH) == BST_CHECKED)
        {
            pTT->TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;

            int iDate = GetDlgItemInt(  hwndDlg,
                                        IDC_CUSTOM_MONTHLY_DAYOFMONTH_DAY_EDIT,
                                        &bTranslated,
                                        FALSE);

            ASSERT(iDate > 0);

            // Day X
            pTT->Type.MonthlyDate.rgfDays = (1 << (iDate - 1));

            // of every Y month(s)
            pTT->Type.MonthlyDate.rgfMonths = GetDlgItemMonth(hwndDlg, IDC_CUSTOM_MONTHLY_DAYOFMONTH_MONTH_EDIT);
        }
        else
        {
            // Monthly -- The X Y of every Z month(s)
            ASSERT(IsDlgButtonChecked(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC) == BST_CHECKED);

            pTT->TriggerType = TASK_TIME_TRIGGER_MONTHLYDOW;

            // The X
            int iWeek = ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_LIST1))+1;
            ASSERT((iWeek >= TASK_FIRST_WEEK) && (iWeek <= TASK_LAST_WEEK));
            pTT->Type.MonthlyDOW.wWhichWeek = iWeek;

            // Y
            int iDaysOfWeek = ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_LIST2));
            ASSERT((iDaysOfWeek >= 0) && (iDaysOfWeek < ARRAYSIZE(s_rgwDaysOfTheWeek)));
            pTT->Type.MonthlyDOW.rgfDaysOfTheWeek = s_rgwDaysOfTheWeek[iDaysOfWeek];

            // of every Z month(s)
            pTT->Type.MonthlyDOW.rgfMonths = GetDlgItemMonth(hwndDlg, IDC_CUSTOM_MONTHLY_PERIODIC_EDIT);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// SetTaskTriggerToDefaults
/////////////////////////////////////////////////////////////////////////////
void SetTaskTriggerToDefaults
(
    TASK_TRIGGER * pTT
)
{
    ASSERT(pTT != NULL);

    // REVIEW: Make this a const static struct and assign instead of function?
    ZeroMemory(pTT, SIZEOF(TASK_TRIGGER));

    pTT->cbTriggerSize              = SIZEOF(TASK_TRIGGER);
    pTT->wStartHour                 = DEFAULT_UPDATETIME_HRS;
    pTT->wStartMinute               = DEFAULT_UPDATETIME_MINS;
    pTT->TriggerType                = TASK_TIME_TRIGGER_DAILY;
    pTT->Type.Daily.DaysInterval    = DEFAULT_DAILY_EVERYNDAYS;
}

/////////////////////////////////////////////////////////////////////////////
// OnDataChanged
/////////////////////////////////////////////////////////////////////////////
void OnDataChanged
(
    HWND            hwndDlg,
    SSUIDLGINFO *   pDlgInfo
)
{
    // Ignore if we are setting up the dialog control values.
    if (pDlgInfo->bInitializing)
        return;

    // Update the TASK_TRIGGER to new user settings.
    SetTaskTriggerFromDlg(hwndDlg, pDlgInfo);

    // Calc and display next update time.
    SetDlgItemNextUpdate(hwndDlg, IDC_CUSTOM_NEXTUPDATE, pDlgInfo);

    // Data changed
    pDlgInfo->bDataChanged = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// ApplyDlgChanges
/////////////////////////////////////////////////////////////////////////////
HRESULT ApplyDlgChanges
(
    HWND            hwndDlg,
    SSUIDLGINFO *   pDlgInfo
)
{
    HRESULT hrResult;

    if (ValidateDlgFields(hwndDlg, pDlgInfo))
    {
        HWND hwndCombo = GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_LIST);

        if (pDlgInfo->dwFlags & SSUI_CREATENEWSCHEDULE)
        {
            if (SUCCEEDED(hrResult = CreateScheduleGroupFromDlg(hwndDlg, pDlgInfo)))
            {
                pDlgInfo->dwFlags |= SSUI_SCHEDULECREATED;

                // Update the combobox with the new group, just
                // in case we aren't closing the dialog.
                TCHAR szGroupName[MAX_GROUPNAME_LEN];
                GetDlgGroupName(hwndDlg, szGroupName, ARRAYSIZE(szGroupName));

                EVAL(SUCCEEDED(SchedGroupComboBox_AddGroup( hwndCombo,
                                                            szGroupName,
                                                            pDlgInfo->pGroupCookie)));
            }
            else
            {
                // REVIEW: Error message! [jaym]
                MessageBeep(MB_ICONEXCLAMATION);
            }
        }
        else
        {
            ASSERT(pDlgInfo->dwFlags & SSUI_EDITSCHEDULE);

            TCHAR szGroupName[MAX_GROUPNAME_LEN];
            GetDlgGroupName(hwndDlg, szGroupName, ARRAYSIZE(szGroupName));

            WCHAR wszGroupName[MAX_GROUPNAME_LEN];
            MyStrToOleStrN(wszGroupName, MAX_GROUPNAME_LEN, szGroupName);

            GROUPINFO gi;
            gi.cbSize = SIZEOF(GROUPINFO);
            gi.pwzGroupname = wszGroupName;

            SetTaskTriggerFromDlg(hwndDlg, pDlgInfo);

#ifdef DEBUG
            DumpTaskTrigger(&pDlgInfo->ttTaskTrigger);
#endif  // DEBUG

            if (SUCCEEDED(hrResult = ModifyScheduleGroup(   pDlgInfo->pGroupCookie,
                                                            &pDlgInfo->ttTaskTrigger,
                                                            NULL,
                                                            &gi,
                                                            0)))
            {
                pDlgInfo->dwFlags |= SSUI_SCHEDULECHANGED;
//xnotfmgr

                // Reset combo list entry because name may have changed.
                NOTIFICATIONCOOKIE cookie;
                EVAL(SUCCEEDED(SchedGroupComboBox_GetCurGroup(hwndCombo, &cookie)));

                EVAL(SUCCEEDED(SchedGroupComboBox_RemoveGroup(hwndCombo, pDlgInfo->pGroupCookie)));
                EVAL(SUCCEEDED(SchedGroupComboBox_AddGroup(hwndCombo, szGroupName, pDlgInfo->pGroupCookie)));

                EVAL(SUCCEEDED(SchedGroupComboBox_SetCurGroup(hwndCombo, &cookie)));
            }
            else
            {
                // REVIEW: Error message! [jaym]
                MessageBeep(MB_ICONEXCLAMATION);
            }
        }
    }
    else
        hrResult = E_FAIL;

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// ValidateDlgFields
/////////////////////////////////////////////////////////////////////////////
BOOL ValidateDlgFields
(
    HWND            hwndDlg,
    SSUIDLGINFO *   pDlgInfo
)
{
    BOOL bResult = FALSE;   // Assume the worst.

    TCHAR szGroupName[MAX_GROUPNAME_LEN];
    GetDlgGroupName(hwndDlg, szGroupName, ARRAYSIZE(szGroupName));

    for (;;)
    {
        int cchGroupNameLen = GetDlgGroupNameLength(hwndDlg);

        // Check length of group name. Tell user if too long.
        if (cchGroupNameLen >= MAX_GROUPNAME_LEN)
        {
            SGMessageBox(   hwndDlg,
                            IDS_CUSTOM_ERROR_NAMETOOLONG,
                            MB_OK | MB_ICONEXCLAMATION);
            break;
        }

        // Make sure we have a description
        if (cchGroupNameLen <= 0)
        {
            SGMessageBox(   hwndDlg,
                            IDS_CUSTOM_ERROR_NAMEREQUIRED,
                            MB_OK | MB_ICONEXCLAMATION);
            break;
        }
        
        // Check to see if group name is already in use.
        if  (
            (
                pDlgInfo->bScheduleNameChanged
                ||
                (pDlgInfo->dwFlags & SSUI_CREATENEWSCHEDULE)
            )
            &&
            ScheduleGroupExists(szGroupName)
            )
        {
            SGMessageBox(   hwndDlg,
                            IDS_CUSTOM_ERROR_NAMEEXISTS,
                            MB_OK | MB_ICONEXCLAMATION);

            break;
        }

        // NOTE: Spin ranges are checked in RestrictDlgItemRange()

        bResult = TRUE;
        break;
    }

    if (!bResult)
    {
        // Set focus to the group name control.
        if (IsWindowVisible(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_EDIT)))
            SetFocus(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_EDIT));
        else
            SetFocus(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_LIST));
    }

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// GetDlgGroupName
/////////////////////////////////////////////////////////////////////////////
UINT GetDlgGroupName
(
    HWND    hwndDlg,
    LPTSTR  pszGroupName,
    int     cchGroupNameLen
)
{
    UINT uResult;

    if (IsWindowVisible(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_EDIT)))
    {
        uResult = GetDlgItemText(   hwndDlg,
                                    IDC_CUSTOM_GROUP_EDIT,
                                    pszGroupName,
                                    cchGroupNameLen);
    }
    else if (IsWindowVisible(GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_LIST)))
    {
        uResult = GetDlgItemText(   hwndDlg,
                                    IDC_CUSTOM_GROUP_LIST,
                                    pszGroupName,
                                    cchGroupNameLen);
    }
    else
        uResult = 0;

    return uResult;
}

/////////////////////////////////////////////////////////////////////////////
// GetDlgGroupNameLength
/////////////////////////////////////////////////////////////////////////////
int GetDlgGroupNameLength
(
    HWND hwndDlg
)
{
    HWND hwndCtl;

    if  (
        IsWindowVisible(hwndCtl = GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_EDIT))
        ||
        IsWindowVisible(hwndCtl = GetDlgItem(hwndDlg, IDC_CUSTOM_GROUP_LIST))
        )
    {
        return GetWindowTextLength(hwndCtl);
    }
    else
        return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CreateScheduleGroupFromDlg
/////////////////////////////////////////////////////////////////////////////
HRESULT CreateScheduleGroupFromDlg
(
    HWND          hwndDlg,
    SSUIDLGINFO * pDlgInfo
)
{
    HRESULT hrResult;

    ASSERT(hwndDlg != NULL);
    ASSERT(IsWindow(hwndDlg));
    ASSERT(pDlgInfo != NULL);

    for (;;)
    {
        *(pDlgInfo->pGroupCookie) = GUID_NULL;
        
        // Get the Trigger info from the dialog control states.
        SetTaskTriggerFromDlg(hwndDlg, pDlgInfo);

#ifdef DEBUG
        DumpTaskTrigger(&pDlgInfo->ttTaskTrigger);
#endif  // DEBUG

        TCHAR szGroupName[MAX_GROUPNAME_LEN];
        GetDlgGroupName(hwndDlg, szGroupName, ARRAYSIZE(szGroupName));

        WCHAR wszGroupName[MAX_GROUPNAME_LEN];
        MyStrToOleStrN(wszGroupName, MAX_GROUPNAME_LEN, szGroupName);

        GROUPINFO gi;
        gi.cbSize = SIZEOF(GROUPINFO);
        gi.pwzGroupname = wszGroupName;

        if (FAILED(hrResult = CreateScheduleGroup(  &pDlgInfo->ttTaskTrigger,
                                                    NULL,
                                                    &gi,
                                                    0,
                                                    pDlgInfo->pGroupCookie)))
        {
            TraceMsg(TF_ERROR, "CSGFD: Unable to CreateScheduleGroup");
            break;
        }

        break;
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// RestrictDlgItemRange
/////////////////////////////////////////////////////////////////////////////
void RestrictDlgItemRange
(
    HWND            hwndDlg,
    int             idCtl,
    SSUIDLGINFO *   pDlgInfo
)
{
    // Special case repeat-every, which is based on the
    // time values of other controls in the dialog.
    if  (
        (idCtl == IDC_CUSTOM_TIME_REPEATEVERY_EDIT)
        &&
        (IsDlgButtonChecked(hwndDlg, IDC_CUSTOM_TIME_REPEATBETWEEN) == BST_CHECKED)
        )
    {
        RestrictTimeRepeatEvery(hwndDlg);
    }
    else
    {
        int     nMin;
        int     nMax;
        int     nDefault;
        BOOL    bTranslated;

        // Get predefined range
        switch (idCtl)
        {
            case IDC_CUSTOM_DAILY_EVERYNDAYS_EDIT:
            {
                nMin = DEFAULT_DAILY_EVERYNDAYS_MIN;
                nMax = DEFAULT_DAILY_EVERYNDAYS_MAX;
                nDefault = DEFAULT_DAILY_EVERYNDAYS;
                break;
            }

            case IDC_CUSTOM_WEEKLY_REPEATWEEKS_EDIT:
            {
                nMin = DEFAULT_WEEKLY_REPEATWEEKS_MIN;
                nMax = DEFAULT_WEEKLY_REPEATWEEKS_MAX;
                nDefault = DEFAULT_WEEKLY_REPEATWEEKS;
                break;
            }

            case IDC_CUSTOM_MONTHLY_DAYOFMONTH_DAY_EDIT:
            {
                SYSTEMTIME stLocal;
                GetLocalTime(&stLocal);

                nMin = DEFAULT_MONTHLY_DAY_MIN;
                nMax = DEFAULT_MONTHLY_DAY_MAX;
                nDefault = stLocal.wDay;
                break;
            }

            case IDC_CUSTOM_MONTHLY_DAYOFMONTH_MONTH_EDIT:
            {
                nMin = DEFAULT_MONTHLY_MONTHS_MIN;
                nMax = DEFAULT_MONTHLY_MONTHS_MAX;
                nDefault = DEFAULT_MONTHLY_MONTHS;
                break;
            }

            case IDC_CUSTOM_MONTHLY_PERIODIC_EDIT:
            {
                nMin = DEFAULT_MONTHLY_MONTHS_MIN;
                nMax = DEFAULT_MONTHLY_MONTHS_MAX;
                nDefault = DEFAULT_MONTHLY_MONTHS;
                break;
            }

            case IDC_CUSTOM_TIME_REPEATEVERY_EDIT:
            {
                nMin = DEFAULT_REPEATUPDATE_HRS_MIN;
                nMax = DEFAULT_REPEATUPDATE_HRS_MAX;
                nDefault = DEFAULT_REPEATUPDATE_HRS;
                break;
            }

            default:
            {
                ASSERT(FALSE);
                break;
            }
        }

        // Get current value
        BOOL    bChanged = FALSE;
        int     nCurrent = (int)GetDlgItemInt(hwndDlg, idCtl, &bTranslated, TRUE);

        if (bTranslated)
        {
            int nRestricted = min(nMax, max(nMin, nCurrent));

            if (nRestricted != nCurrent)
                SetDlgItemInt(hwndDlg, idCtl, nRestricted, TRUE);
        }
        else
            SetDlgItemInt(hwndDlg, idCtl, nDefault, TRUE);

    }
}

/////////////////////////////////////////////////////////////////////////////
// RestrictTimeRepeatEvery
/////////////////////////////////////////////////////////////////////////////
void RestrictTimeRepeatEvery
(
    HWND hwndDlg
)
{
    SYSTEMTIME st = { 0 };
    DateTime_GetSystemtime( GetDlgItem( hwndDlg, 
                                        IDC_CUSTOM_TIME_REPEATBETWEEN_START_TIME),
                            &st);
    WORD wStartMins = (st.wHour * 60) + st.wMinute;

    DateTime_GetSystemtime( GetDlgItem( hwndDlg,
                                        IDC_CUSTOM_TIME_REPEATBETWEEN_END_TIME),
                            &st);
    WORD wEndMins = (st.wHour * 60) + st.wMinute;

    WORD nHours;
    if (wStartMins > wEndMins)
        nHours = ((1440 - wStartMins) + wEndMins) / 60;
    else
        nHours = (wEndMins - wStartMins) / 60;

    if (nHours <= 0)
        nHours = DEFAULT_REPEATUPDATE_HRS_MIN;

    UpDown_SetRange(GetDlgItem(hwndDlg, IDC_CUSTOM_TIME_REPEATEVERY_SPIN),
                    DEFAULT_REPEATUPDATE_HRS_MIN, nHours);

    BOOL bTranslated;
    int nRepeatEvery = GetDlgItemInt(   hwndDlg,
                                        IDC_CUSTOM_TIME_REPEATEVERY_EDIT,
                                        &bTranslated,
                                        FALSE);

    int nRestricted = min(nHours, max(1, nRepeatEvery));
    if (nRestricted != nRepeatEvery)
    {
        SetDlgItemInt(  hwndDlg,
                        IDC_CUSTOM_TIME_REPEATEVERY_EDIT,
                        nRestricted,
                        FALSE);
    }
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// GetSchedGroupTaskTrigger
/////////////////////////////////////////////////////////////////////////////
HRESULT GetSchedGroupTaskTrigger
(
    PNOTIFICATIONCOOKIE pGroupCookie,
    TASK_TRIGGER *      pTT
)
{
//xnotfmgr

    INotificationMgr *  pNotificationMgr = NULL;
    IScheduleGroup *    pScheduleGroup = NULL;
    HRESULT             hrResult;

    ASSERT(pGroupCookie != NULL);
    ASSERT(pTT != NULL);
    
    for (;;)
    {
        ZeroMemory(pTT, SIZEOF(TASK_TRIGGER));
        pTT->cbTriggerSize = SIZEOF(TASK_TRIGGER);

        if (FAILED(hrResult = GetNotificationMgr(&pNotificationMgr)))
        {
            TraceMsg(TF_ERROR, "GSGTaskTrigger: Unable to get NotificationMgr");
            break;
        }

        if (FAILED(hrResult = pNotificationMgr->FindScheduleGroup(  pGroupCookie,
                                                                    &pScheduleGroup,
                                                                    0)))
        {
            TraceMsg(TF_ERROR, "GSGTaskTrigger: Unable to locate schedule group");
            break;
        }

        if (FAILED(hrResult = pScheduleGroup->GetAttributes(pTT,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            NULL)))
        {
            TraceMsg(TF_ERROR, "GSGTaskTrigger: Unable to get schedule group attributes");
            break;
        }

        break;
    }

    SAFERELEASE(pScheduleGroup);
    SAFERELEASE(pNotificationMgr);

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CreateScheduleGroup
/////////////////////////////////////////////////////////////////////////////
HRESULT CreateScheduleGroup
(
    /* [in] */  PTASK_TRIGGER       pTaskTrigger,
    /* [in] */  PTASK_DATA          pTaskData,
    /* [in] */  PGROUPINFO          pGroupInfo,
    /* [in] */  GROUPMODE           grfGroupMode,
    /* [out] */ PNOTIFICATIONCOOKIE pGroupCookie
)
{
//xnotfmgr

    INotificationMgr *  pNotificationMgr = NULL;
    IScheduleGroup *    pScheduleGroup = NULL;
    HRESULT             hrResult;

    for (;;)
    {
        if (pGroupCookie == NULL)
            return E_INVALIDARG;

        if (FAILED(hrResult = GetNotificationMgr(&pNotificationMgr)))
        {
            TraceMsg(TF_ERROR, "CSG: Unable to get NotificationMgr");
            break;
        }

        if (FAILED(hrResult = pNotificationMgr->CreateScheduleGroup(0,
                                                                    &pScheduleGroup,
                                                                    pGroupCookie,
                                                                    0)))
        {
            TraceMsg(TF_ERROR, "CSG: Unable to CreateScheduleGroup");
            break;
        }

        ASSERT(!pTaskData);
        if (FAILED(hrResult = pScheduleGroup->SetAttributes(pTaskTrigger,
                                                            pTaskData,
                                                            pGroupCookie,
                                                            pGroupInfo,
                                                            grfGroupMode)))
        {
            TraceMsg(TF_ERROR, "CSG: Unable to SetAttributes");
            break;
        }

        break;
    }

    SAFERELEASE(pScheduleGroup);
    SAFERELEASE(pNotificationMgr);

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// ModifyScheduleGroup
/////////////////////////////////////////////////////////////////////////////
HRESULT ModifyScheduleGroup
(
    /* [in] */  PNOTIFICATIONCOOKIE pGroupCookie,
    /* [in] */  PTASK_TRIGGER       pTaskTrigger,
    /* [in] */  PTASK_DATA          pTaskData,
    /* [in] */  PGROUPINFO          pGroupInfo,
    /* [in] */  GROUPMODE           grfGroupMode
)
{
//xnotfmgr

    INotificationMgr *  pNotificationMgr = NULL;
    IScheduleGroup *    pScheduleGroup = NULL;
    HRESULT             hrResult;

    // This might take a while.
    WAITCURSOR Hourglass;

    for (;;)
    {
        if  (
            (pGroupCookie == NULL)
            ||
            (*pGroupCookie == GUID_NULL)
            )
        {
            return E_INVALIDARG;
        }

        if (FAILED(hrResult = GetNotificationMgr(&pNotificationMgr)))
        {
            TraceMsg(TF_ERROR, "MSG: Unable to get NotificationMgr");
            break;
        }

        if (FAILED(hrResult = pNotificationMgr->FindScheduleGroup(  pGroupCookie,
                                                                    &pScheduleGroup,
                                                                    0)))
        {
            TraceMsg(TF_ERROR, "MSG: Unable to locate schedule group");
            break;
        }

        ASSERT(!pTaskData);
        if (FAILED(hrResult = pScheduleGroup->SetAttributes(pTaskTrigger,
                                                            pTaskData,
                                                            pGroupCookie,
                                                            pGroupInfo,
                                                            grfGroupMode)))
        {
            TraceMsg(TF_ERROR, "MSG: Unable to SetAttributes");
            break;
        }

        {
            LPENUMNOTIFICATION  pEnum = NULL;
            NOTIFICATIONITEM    item = {0};
            OOEBuf  ooeBuf;
            HRESULT hr;

            if (FAILED(hr = pScheduleGroup->GetEnumNotification(0, &pEnum)))
            {
                TraceMsg(TF_ERROR, "MSG: Unable to GetEnumNotification");
                break;
            }

            memset(&ooeBuf, 0, sizeof(ooeBuf));

            item.cbSize = sizeof(NOTIFICATIONITEM);
            ULONG   cItems = 0;
            DWORD   dwBufSize = 0;

            hr = pEnum->Next(1, &item, &cItems);
            while (SUCCEEDED(hr) && cItems) {
                ASSERT(item.pNotification);
                hr = LoadOOEntryInfo(&ooeBuf, &item, &dwBufSize);
                SAFERELEASE(item.pNotification);
                if (hr == S_OK) {
                    LPMYPIDL pooi = COfflineFolderEnum::NewPidl(dwBufSize);
                    if (pooi)   {
                        CopyToMyPooe(&ooeBuf, &(pooi->ooe));
                        _GenerateEvent( SHCNE_UPDATEITEM,
                                        (LPITEMIDLIST)pooi,
                                        NULL);
                        COfflineFolderEnum::FreePidl(pooi);
                    } else  {
                        hr = E_OUTOFMEMORY;     //  BUGBUG  Ignore it?
                    }
                }
                ZeroMemory(&ooeBuf, sizeof(ooeBuf));
                item.cbSize = sizeof(NOTIFICATIONITEM);
                cItems = 0;
                dwBufSize = 0;

                hr = pEnum->Next(1, &item, &cItems);
            }
            SAFERELEASE(item.pNotification);
            SAFERELEASE(pEnum);
        }

        break;
    }

    SAFERELEASE(pScheduleGroup);
    SAFERELEASE(pNotificationMgr);

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// DeleteScheduleGroup
/////////////////////////////////////////////////////////////////////////////
HRESULT DeleteScheduleGroup
(
    /* [in] */  PNOTIFICATIONCOOKIE pGroupCookie
)
{
//xnotfmgr

    INotificationMgr *  pNotificationMgr = NULL;
    HRESULT             hrResult;

    for (;;)
    {
        if  (
            (pGroupCookie == NULL)
            ||
            (*pGroupCookie == GUID_NULL)
            )
        {
            return E_INVALIDARG;
        }

        if (FAILED(hrResult = GetNotificationMgr(&pNotificationMgr)))
        {
            TraceMsg(TF_ERROR, "DSG: Unable to get NotificationMgr");
            break;
        }

        if (FAILED(hrResult = pNotificationMgr->RevokeScheduleGroup(pGroupCookie,
                                                                    NULL,
                                                                    0)))
        {
            TraceMsg(TF_ERROR, "DSG: RevokeScheduleGroup FAILED");
            break;
        }

        break;
    }

    SAFERELEASE(pNotificationMgr);

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// ScheduleGroupExists
/////////////////////////////////////////////////////////////////////////////
BOOL ScheduleGroupExists
(
    LPCTSTR lpszGroupName
)
{
//xnotfmgr
    INotificationMgr *      pNotificationMgr = NULL;
    IEnumScheduleGroup *    pEnumSchedGroup = NULL;
    BOOL                    bResult = FALSE;

    for (;;)
    {
        if (FAILED(GetNotificationMgr(&pNotificationMgr)))
        {
            TraceMsg(TF_ERROR, "SGE: Unable to get NotificationMgr");
            break;
        }

        if (FAILED(pNotificationMgr->GetEnumScheduleGroup(0, &pEnumSchedGroup)))
        {
            TraceMsg(TF_ERROR, "SGE: Unable to GetEnumScheduleGroup");
            break;
        }

        // Iterate through all the schedule groups
        ULONG               celtFetched;
        IScheduleGroup *    pScheduleGroup = NULL;
        while   (
                SUCCEEDED(pEnumSchedGroup->Next(1,
                                                &pScheduleGroup,
                                                &celtFetched))
                &&
                (celtFetched != 0)
                )
        {
            GROUPINFO info = { 0 };

            ASSERT(pScheduleGroup != NULL);

            // Get the schedule group attributes.
            if  (
                SUCCEEDED(pScheduleGroup->GetAttributes(NULL,
                                                        NULL,
                                                        NULL,
                                                        &info,
                                                        NULL,
                                                        NULL))
                )
            {
                ASSERT(info.cbSize == SIZEOF(GROUPINFO));
                ASSERT(info.pwzGroupname != NULL);

                TCHAR szGroupName[MAX_GROUPNAME_LEN];
                MyOleStrToStrN(szGroupName, MAX_GROUPNAME_LEN, info.pwzGroupname);
                SAFEDELETE(info.pwzGroupname);

                // See if this is the schedule group name we are looking for.
                if (StrCmpI(lpszGroupName, szGroupName) == 0)
                {
                    bResult = TRUE;
                    break;
                }
            }
            else
                TraceMsg(TF_ERROR, "SGE: Unable to GetAttributes");

            SAFERELEASE(pScheduleGroup);
        }

        break;
    }

    SAFERELEASE(pEnumSchedGroup);
    SAFERELEASE(pNotificationMgr);

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// SetDlgItemMonth
/////////////////////////////////////////////////////////////////////////////
BOOL SetDlgItemMonth
(
    HWND    hwndDlg,
    int     idCtl,
    WORD    rgfMonths
)
{
    return SetDlgItemInt(   hwndDlg,
                            idCtl,
                            MonthCountFromMonthsFlags(rgfMonths),
                            FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// MonthCountFromMonthsFlags
/////////////////////////////////////////////////////////////////////////////
int MonthCountFromMonthsFlags
(
    WORD rgfMonths
)
{
    for (int iMonths = 0; iMonths < ARRAYSIZE(c_rgwMonthMaps); iMonths++)
    {
        if (c_rgwMonthMaps[iMonths] == rgfMonths)
            break;
    }

    // If we didn't find it, assume every month
    if (iMonths == ARRAYSIZE(c_rgwMonthMaps))
        iMonths = 0;

    return iMonths+1;
}

/////////////////////////////////////////////////////////////////////////////
// DayIndexFromDaysOfTheWeekFlags
/////////////////////////////////////////////////////////////////////////////
int DayIndexFromDaysOfTheWeekFlags
(
    WORD rgfDaysOfTheWeek
)
{
    int idxDay;

    for (idxDay = 0; idxDay < ARRAYSIZE(s_rgwDaysOfTheWeek); idxDay++)
    {
        if (s_rgwDaysOfTheWeek[idxDay] == rgfDaysOfTheWeek)
            break;
    }

    if (idxDay == ARRAYSIZE(s_rgwDaysOfTheWeek))
        idxDay = 0;

    return idxDay;
}

/////////////////////////////////////////////////////////////////////////////
// GetDlgItemMonth
/////////////////////////////////////////////////////////////////////////////
WORD GetDlgItemMonth
(
    HWND    hwndDlg,
    int     idCtl
)
{
    BOOL bTranslated;

    int iVal = GetDlgItemInt(   hwndDlg,
                                idCtl,
                                &bTranslated,
                                FALSE);

    if  (
        bTranslated
        &&
        ((iVal > 0) && (iVal <= ARRAYSIZE(c_rgwMonthMaps)))
        )
    {
        return c_rgwMonthMaps[iVal-1];
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// SetDlgItemNextUpdate
/////////////////////////////////////////////////////////////////////////////
void SetDlgItemNextUpdate
(
    HWND            hwndDlg,
    int             idCtl,
    SSUIDLGINFO *   pDlgInfo
)
{
//xnotfmgr

    WORD        cDate = 1;
    FILETIME    dateNextUpdate;
    TCHAR       szNextUpdate[128];

    SYSTEMTIME  stBegin;
    GetLocalTime(&stBegin);

    if  (
        (pDlgInfo->pfnGetRunTimes != NULL)
        &&
        SUCCEEDED(pDlgInfo->pfnGetRunTimes( pDlgInfo->ttTaskTrigger,
                                            NULL,
                                            &stBegin,
                                            NULL,
                                            &cDate,
                                            &dateNextUpdate))
        &&
        (cDate != 0)
        )
    {
        FileTimeToSystemTime(&dateNextUpdate, &stBegin);
    }
    else
        GetLocalTime(&stBegin);

    LPTSTR pszNextUpdate = szNextUpdate;
    GetDateFormat(  LOCALE_USER_DEFAULT,
                    DATE_LONGDATE,
                    &stBegin,
                    NULL,
                    pszNextUpdate,
                    ARRAYSIZE(szNextUpdate) / 2);

    pszNextUpdate += lstrlen(pszNextUpdate); *pszNextUpdate++ = TEXT(' ');

    GetTimeFormat(  LOCALE_USER_DEFAULT,
                    TIME_NOSECONDS,
                    &stBegin,
                    NULL,
                    pszNextUpdate,
                    ARRAYSIZE(szNextUpdate) / 2);

    SetDlgItemText(hwndDlg, idCtl, szNextUpdate);
}

/////////////////////////////////////////////////////////////////////////////
// IncTime
/////////////////////////////////////////////////////////////////////////////
DWORD IncTime
(
    DWORD   dwTime,
    int     iIncHours,
    int     iIncMinutes
)
{
    int iMins = (LOWORD(dwTime) * 60) + HIWORD(dwTime);
    int iIncMins = (iIncHours * 60) + iIncMinutes;
    int iDeltaMins = iMins + iIncMins;

    if (iDeltaMins > ((24 * 60) - 1))
        iDeltaMins = iDeltaMins - (24 * 60);
    else if (iDeltaMins < 0)
        iDeltaMins = ((24 * 60) - 1);

    return MAKELONG((iDeltaMins / 60), (iDeltaMins % 60));
}

/////////////////////////////////////////////////////////////////////////////
// DayFromDaysFlags
/////////////////////////////////////////////////////////////////////////////
int DayFromDaysFlags
(
    DWORD rgfDays
)
{
    DWORD   dwMask = 1;
    int     iDay = 1;

    while (dwMask)
    {
        if (rgfDays & dwMask)
            break;

        dwMask <<= 1;
        iDay++;
    }

    if (iDay > 31)
        iDay = 1;

    return iDay;
}

/////////////////////////////////////////////////////////////////////////////
// Schedule group combo box helper functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// SchedGroupComboBox_Fill
/////////////////////////////////////////////////////////////////////////////
HRESULT SchedGroupComboBox_Fill
(
    HWND hwndCombo
)
{
//xnotfmgr
    INotificationMgr *      pNotificationMgr = NULL;
    IEnumScheduleGroup *    pEnumSchedGroup = NULL;
    HRESULT                 hrResult;

    ASSERT(hwndCombo != NULL);
    ASSERT(IsWindow(hwndCombo));

    for (;;)
    {
        SchedGroupComboBox_Clear(hwndCombo);

        if (FAILED(hrResult = GetNotificationMgr(&pNotificationMgr)))
        {
            TraceMsg(TF_ERROR, "FCBSG: Unable to get NotificationMgr");
            break;
        }

        if (FAILED(hrResult = pNotificationMgr->GetEnumScheduleGroup(0, &pEnumSchedGroup)))
        {
            TraceMsg(TF_ERROR, "FCBSG: Unable to GetEnumScheduleGroup");
            break;
        }

        // Iterate through all the schedule gropus
        ULONG               celtFetched;
        IScheduleGroup *    pScheduleGroup = NULL;
        while   (
                SUCCEEDED(hrResult = pEnumSchedGroup->Next( 1,
                                                            &pScheduleGroup,
                                                            &celtFetched))
                &&
                (celtFetched != 0)
                )
        {
            GROUPINFO           info = { 0 };
            NOTIFICATIONCOOKIE  cookie = { 0 };

            ASSERT(pScheduleGroup != NULL);

            // Get the schedule group attributes.
            if  (
                SUCCEEDED(pScheduleGroup->GetAttributes(NULL,
                                                        NULL,
                                                        &cookie,
                                                        &info,
                                                        NULL,
                                                        NULL))
                &&
                (cookie != NOTFCOOKIE_SCHEDULE_GROUP_MANUAL)
                )
            {
                ASSERT(info.cbSize == SIZEOF(GROUPINFO));
                ASSERT(info.pwzGroupname != NULL);

                TCHAR szGroupName[MAX_GROUPNAME_LEN];
                MyOleStrToStrN(szGroupName, MAX_GROUPNAME_LEN, info.pwzGroupname);
                SAFEDELETE(info.pwzGroupname);

                EVAL(SUCCEEDED(SchedGroupComboBox_AddGroup( hwndCombo,
                                                            szGroupName,
                                                            &cookie)));
            }
            else
                TraceMsg(TF_ERROR, "FCBSG: Unable to GetAttributes");

            SAFERELEASE(pScheduleGroup);
        }

        break;
    }

    SAFERELEASE(pEnumSchedGroup);
    SAFERELEASE(pNotificationMgr);

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// SchedGroupComboBox_Clear
/////////////////////////////////////////////////////////////////////////////
HRESULT SchedGroupComboBox_Clear
(
    HWND hwndCombo
)
{
//xnotfmgr
    ASSERT(hwndCombo != NULL);
    ASSERT(IsWindow(hwndCombo));

    for (int i = 0; i < ComboBox_GetCount(hwndCombo); i++)
    {
        PNOTIFICATIONCOOKIE pCookie = (PNOTIFICATIONCOOKIE)ComboBox_GetItemData(hwndCombo, i);

        if ((pCookie != NULL) && ((DWORD)pCookie != CB_ERR))
            MemFree(pCookie);
    }

    ComboBox_ResetContent(hwndCombo);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// SchedGroupComboBox_AddGroup
/////////////////////////////////////////////////////////////////////////////
HRESULT SchedGroupComboBox_AddGroup
(
    HWND                hwndCombo,
    LPCTSTR             pszGroupName,
    PNOTIFICATIONCOOKIE pGroupCookie
)
{
//xnotfmgr
    ASSERT(hwndCombo != NULL);
    ASSERT(IsWindow(hwndCombo));
    ASSERT(pszGroupName != NULL);
    ASSERT(pGroupCookie != NULL);

    // Add the schedule group name to the combobox.
    int iIndex = ComboBox_AddString(hwndCombo, pszGroupName);
    if ((iIndex != CB_ERR) && (iIndex != CB_ERRSPACE))
    {
        // Attach the schedule group cookie to the item data.
        PNOTIFICATIONCOOKIE pCookie = (PNOTIFICATIONCOOKIE)MemAlloc(LPTR,
                                                                    SIZEOF(NOTIFICATIONCOOKIE));
        if (pCookie == NULL)
            return E_OUTOFMEMORY;

        *pCookie = *pGroupCookie;
        ComboBox_SetItemData(hwndCombo, iIndex, (DWORD)pCookie);

        return S_OK;
    }
    else

    ASSERT(FALSE);
    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
// SchedGroupComboBox_RemoveGroup
/////////////////////////////////////////////////////////////////////////////
HRESULT SchedGroupComboBox_RemoveGroup
(
    HWND                hwndCombo,
    PNOTIFICATIONCOOKIE pGroupCookie
)
{
//xnotfmgr
    ASSERT(hwndCombo != NULL);
    ASSERT(IsWindow(hwndCombo));
    ASSERT(pGroupCookie != NULL);
    ASSERT(*pGroupCookie != GUID_NULL);

    for (int i = 0 ; i < ComboBox_GetCount(hwndCombo); i++)
    {
        PNOTIFICATIONCOOKIE pCookie = (PNOTIFICATIONCOOKIE)ComboBox_GetItemData(hwndCombo, i);

        ASSERT(pCookie != NULL);

        if (*pCookie == *pGroupCookie)
        {
            MemFree(pCookie);

            EVAL(ComboBox_DeleteString(hwndCombo, i));
            return S_OK;
        }
    }

    ASSERT(FALSE);
    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
// SchedGroupComboBox_SetCurGroup
/////////////////////////////////////////////////////////////////////////////
HRESULT SchedGroupComboBox_SetCurGroup
(
    HWND                hwndCombo,
    PNOTIFICATIONCOOKIE pGroupCookie
)
{
//xnotfmgr
    ASSERT(hwndCombo != NULL);
    ASSERT(IsWindow(hwndCombo));
    if (pGroupCookie == NULL || *pGroupCookie == GUID_NULL)
        return E_INVALIDARG;

    for (int i = 0 ; i < ComboBox_GetCount(hwndCombo); i++)
    {
        PNOTIFICATIONCOOKIE pData = (PNOTIFICATIONCOOKIE)ComboBox_GetItemData(hwndCombo, i);

        if (pData && *pData == *pGroupCookie)
        {
            EVAL(ComboBox_SetCurSel(hwndCombo, i) != -1);
            return S_OK;
        }
    }

    ASSERT(FALSE);
    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
// SchedGroupComboBox_GetCurGroup
/////////////////////////////////////////////////////////////////////////////
HRESULT SchedGroupComboBox_GetCurGroup
(
    HWND                hwndCombo,
    PNOTIFICATIONCOOKIE pGroupCookie
)
{
//xnotfmgr
    ASSERT(hwndCombo != NULL);
    ASSERT(IsWindow(hwndCombo));
    ASSERT(pGroupCookie != NULL);

    int iIndex = ComboBox_GetCurSel(hwndCombo);
    if (iIndex != CB_ERR)
    {
        PNOTIFICATIONCOOKIE pCookie = (PNOTIFICATIONCOOKIE)ComboBox_GetItemData(hwndCombo, iIndex);

        if (pCookie == NULL)
            return E_INVALIDARG;

        *pGroupCookie = *pCookie;

        return S_OK;
    }

    ASSERT(FALSE);
    return E_FAIL;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// wrappers for schedui combobox functions, to fill list and get current selection,
// that are aware of a NULL itemdata field representing the Publisher's Recommended
// Schedule entry (NULL itemdata means no groupCookie means use task trigger).
// 
////////////////////////////////////////////////////////////////////////////////

HRESULT FillScheduleList (HWND hwndCombo, POOEBuf pBuf)
{
    //first, fill with schedule groups
    HRESULT hr = SchedGroupComboBox_Fill (hwndCombo);

    //then, if channel, add "publisher schedule" to list
    if (pBuf->bChannel)
    {
        TCHAR tcBuf[128];

        MLLoadString(IDS_SCHEDULE_PUBLISHER, tcBuf, ARRAYSIZE(tcBuf));
        ComboBox_AddString (hwndCombo, tcBuf);
        //leave itemdata (cookie ptr) NULL
    }

    return hr;
}

HRESULT SetScheduleGroup (HWND hwndCombo, CLSID* pGroupCookie, POOEBuf pBuf)
{
//xnotfmgr
    HRESULT hr;

    if (pBuf->bChannel && (pBuf->fChannelFlags & CHANNEL_AGENT_DYNAMIC_SCHEDULE))
        hr = E_FAIL;    //force to Publisher's Schedule
    else
        hr = SchedGroupComboBox_SetCurGroup (hwndCombo, pGroupCookie);

    if (FAILED (hr))
    {
        if (pBuf->bChannel)
        {
            //we are a channel; look for Publisher's Schedule (null cookie ptr)
            for (int i = 0; i < ComboBox_GetCount (hwndCombo); i++)
            {
                if (!ComboBox_GetItemData(hwndCombo, i))
                {
                    EVAL(ComboBox_SetCurSel(hwndCombo, i) != -1);
                    return S_OK;
                }
            }
            hr = E_FAIL;
        }
    }

    return hr;
}
