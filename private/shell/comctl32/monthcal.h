#define CAL_COLOR_TODAY     0x000000ff

#define CALMONTHMAX     12
#define CALROWMAX       6
#define CALCOLMAX       7
#define CAL_DEF_SELMAX  7

// BUGBUG raymondc - these metrics do not scale with user settings
#define CALBORDER       6

//  The formulas for DX_ARROWMARGIN and D[XY]_CALARROW are chosen so on most
//  systems they come out approximately equal to the values you got
//  in IE4.  (The IE4 values were hard-coded and therefore incompatible
//  with accessibility.)

#define DX_ARROWMARGIN      (5 * g_cxBorder)
#define DX_CALARROW         (g_cyHScroll * 4 / 3)
#define DY_CALARROW         g_cyHScroll

#define DXRING_SPIRAL       8
#define DXEDGE_SPIRAL       8

// BUGBUG raymondc - msecautospin should scale on doubleclicktime
#define CAL_MSECAUTOSPIN        350
#define CAL_SECTODAYTIMER       (2 * 60)
#define CAL_IDAUTOSPIN          1
#define CAL_TODAYTIMER          2

#define CCHMAXMONTH     42
#define CCHMAXABBREVDAY 11
#define CCHMAXMARK      10

#define SEL_BEGIN       1
#define SEL_END         2
#define SEL_DOT         3
#define SEL_MID         4

//
//  For each month we display, we have to compute a bunch of metrics
//  to track the stuff we put into the header area.
//
//  The five values represent the following points in the string:
//
//          Mumble January Mumble 1999 Mumble
//         |      |       |      |    |
//         |      |     MonthEnd |    YearEnd
//         Start  MonthStart     YearStart
//
//  Note that it is possible for YearStart to be less than MonthStart if the
//  year comes before the month.  (e.g., "1999 January")
//
//  These values already take RTL mirroring into account.
//
//  NOTE!  IMM_MONTHSTART and IMM_YEARSTART are also used as flags,
//  so they both need to be powers of 2.
//
#define IMM_START        0
#define IMM_DATEFIRST    1
#define IMM_MONTHSTART   1
#define IMM_YEARSTART    2
#define IMM_MONTHEND     3
#define IMM_YEAREND      4
#define IMM_DATELAST     4
#define DMM_STARTEND    2           // Difference between START and END
#define CCH_MARKERS      4          // There are four markers

typedef struct MONTHMETRICS {
    int     rgi[5];
} MONTHMETRICS, *PMONTHMETRICS;

// This stuff used to be global
typedef struct tagLOCALEINFO {
    TCHAR szToday[32];        // "Today:"
    TCHAR szGoToToday[64];    // "&Go to today"

    TCHAR szMonthFmt[8];      // "MMMM"
    TCHAR szMonthYearFmt[16+CCH_MARKERS]; // "\1MMMM\3 \2yyyy\4" -- see MCInsertMarkers

    TCHAR rgszMonth[12][CCHMAXMONTH];
    TCHAR rgszDay[7][CCHMAXABBREVDAY];
    int dowStartWeek;       // LOCALE_IFIRSTDAYOFWEEK (0 = mon, 1 = tue, 6 = sat)
    int firstWeek;          // LOCALE_IFIRSTWEEKOFYEAR

    TCHAR *rgpszMonth[12];  // pointers into rgszMonth
    TCHAR *rgpszDay[7];     // pointers into rgszDay
} LOCALEINFO, *PLOCALEINFO, *LPLOCALEINFO;


//
// SUBEDITCONTROL stuff
//

//
// Note: SECIncrFocus assumes that SUBEDIT_NONE is numerical value -1
//
#define SUBEDIT_NONE -1 // no field is being edited
#define SUBEDIT_ALL  -2 // all fields are being edited (DTS_APPCANPARSE)
enum {
    SE_ERA = 1,    
    SE_YEAR,
    SE_YEARALT,         // see SEGetTimeDateFormat
    SE_MONTH,
    SE_MONTHALT,        // see SEGetTimeDateFormat
    SE_DAY,
    SE_DATELAST = SE_DAY,
    SE_MARK,            // "AM" or "PM" indicator
    SE_HOUR,
    SE_MINUTE,
    SE_SECOND,
    SE_STATIC,
    SE_APP,
    SE_MAX
};

#define SE_YEARLIKE(s)      ((s) == SE_YEAR || (s) == SE_YEARALT)
#define SE_DATELIKE(s)      InRange(s, SE_ERA, SE_DATELAST)

typedef struct tagSUBEDIT {
    int     id;         // SE_ value above
    RECT    rc;         // bounding box for display

    LPWORD  pval;       // current value (in a SYSTEMTIME struct)
    UINT    min;        // min value
    UINT    max;        // max value
    int     cIncrement; // increment value

    int     cchMax;     // max allowed chars
    int     cchEdit;    // current number chars entered so far
    UINT    valEdit;    // value entered so far
    UINT    flDrawText; // flags for DrawText

    LPCTSTR pv;         // formatting string

    BOOL    fReadOnly;  // can this subedit be edited (receive focus)?
} SUBEDIT, * PSUBEDIT, *LPSUBEDIT;

//
//  There are three types of calendars we support
//
//  -   Gregorian (Western).  Any calendar not otherwise supported is forced
//      into Gregorian mode.
//
//  -   Offset.  The year is merely a fixed offset from the Gregorian year.
//      This is the style used by the Korean and Thai calendars.
//
//  -   Era.  The calendar consists of multiple eras, and the year is
//      relative to the start of the enclosing era.  This is the style
//      used by the Japan and Taiwan calendars.  Eras are strangest because
//      an era need not start on January 1!
//
typedef struct tagCALENDARTYPE {
    CALID   calid;        // Calendar id number (CAL_GREGORIAN, etc.)
    LCID    lcid;         // Usually LOCALE_USER_DEFAULT, but forced to US for unsupported calendars
    int     dyrOffset;    // The calendar offset (0 for Gregorian and Era)
    HDPA    hdpaYears;    // If fEra, then array of year info
    HDPA    hdpaEras;     // If fEra, then array of era names
} CALENDARTYPE, *PCALENDARTYPE;

#define BUDDHIST_BIAS   543
#define KOREAN_BIAS     2333

#define ISERACALENDAR(pct)            ((pct)->hdpaEras)

#define GregorianToOther(pct, yr)     ((yr) + (pct)->dyrOffset)
#define OtherToGregorian(pct, yr)     ((yr) - (pct)->dyrOffset)

typedef struct tagSUBEDITCONTROL {
    LPCONTROLINFO pci;  // looks like this guy needs access to the hwnd
    BOOL fNone;         // allow scrolling into SUBEDIT_NONE
    HFONT hfont;        // font to draw text with
    RECT rc;            // rect for subedits
    int xScroll;        // amount pse array is scrolled
    int iseCur;         // subedit with current selection (SUBEDIT_NONE for no selection)
    int cse;            // count of subedits in pse array
    SYSTEMTIME st;      // current time pse represents (pse points into this)
    LPTSTR szFormat;    // format string as parsed (pse points into this)
    PSUBEDIT pse;       // subedit array
    TCHAR   cDelimeter; // delimiter between subedits (parsed from fmt string)
    TCHAR szDelimeters[15]; // delimiters between date/time fields (from resfile)
    CALENDARTYPE ct;    // information about the calendar
    BITBOOL fMirrorSEC:1; // Whether or not to mirror the SubEditControls
    BITBOOL fSwapTimeMarker:1; // Whether we need to swap the AM/PM symbol around or not
} SUBEDITCONTROL, * PSUBEDITCONTROL, *LPSUBEDITCONTROL;

#define SECYBORDER 2
#define SECXBORDER 2

/*
 *    Multiple Month Calendar Control
 */
typedef struct tagMONTHCAL {
    CONTROLINFO ci;     // all controls start with this
    LOCALEINFO li;      // stuff that used to be global

    HINSTANCE hinstance;

    HWND    hwndEdit;   // non-NULL iff dealing with user-click on year
    HWND    hwndUD;     // UpDown control associated with the hwndEdit

    HPEN    hpen;
    HPEN    hpenToday;

    HFONT   hfont;                // stock font, don't destroy
    HFONT   hfontBold;            // created font, so we need to destroy
    
    COLORREF clr[MCSC_COLORCOUNT];    
    
    int     dxCol;             // font info, based on bold to insure that we get enough space
    int     dyRow;
    int     dxMonth;
    int     dyMonth;
    int     dxYearMax;
    int     dyToday;
    int     dxToday;

    int     dxArrowMargin;
    int     dxCalArrow;
    int     dyCalArrow;

    HMENU   hmenuCtxt;
    HMENU   hmenuMonth;

    SYSTEMTIME  stMin;          // minimum selectable date
    SYSTEMTIME  stMax;          // maximum selectable date

    DWORD   cSelMax;

    SYSTEMTIME  stToday;
    SYSTEMTIME  st;             // the selection if not multiselect
                                // the beginning of the selection if multiselect
    SYSTEMTIME  stEndSel;       // the end of the selection if multiselect
    SYSTEMTIME  stStartPrev;    // prev selection beginning (only in multiselect)
    SYSTEMTIME  stEndPrev;      // prev selection end (only in multiselect)

    SYSTEMTIME  stAnchor;       // anchor date in shift-click selection

    SYSTEMTIME  stViewFirst;    // first visible date (DAYSTATE - grayed out)
    SYSTEMTIME  stViewLast;     // last visible date (DAYSTATE - grayed out)
    
    SYSTEMTIME  stMonthFirst;   // first month (stMin adjusted)
    SYSTEMTIME  stMonthLast;    // last month (stMax adjusted)
    int         nMonths;        // number of months being shown (stMonthFirst..stMonthLast)

    UINT_PTR    idTimer;
    UINT_PTR    idTimerToday;

    int     nViewRows;          // number of rows of months shown
    int     nViewCols;          // number of columns of months shown

    RECT    rcPrev;             // rect for prev month button (in window coords)
    RECT    rcNext;             // rect for next month button (in window coords)

    RECT    rcMonthName;        // rect for the month name (in relative coords)
                                // (actually, the rect for the titlebar area of
                                // each month).

    RECT    rcDow;              // rect for days of week (in relative coords)
    RECT    rcWeekNum;          // rect for week numbers (in relative coords)
    RECT    rcDayNum;           // rect for day numbers  (in relative coords)

    int     iMonthToday;
    int     iRowToday;
    int     iColToday;

    RECT    rcDayCur;            // rect for the current selected day
    RECT    rcDayOld;

    RECT    rc;                  // window rc.
    RECT    rcCentered;          // rect containing the centered months

    // The following 4 ranges hold info about the displayed (DAYSTATE) months:
    // They are filled in from 0 to nMonths+1 by MCUpdateStartEndDates
    // NOTE: These are _one_ based indexed arrays of the displayed months    
    int     rgcDay[CALMONTHMAX + 2];    // # days in this month
    int     rgnDayUL[CALMONTHMAX + 2];  // last day in this month NOT visible when viewing next month

    int     dsMonth;             // first month stored in rgdayState
    int     dsYear;              // first year stored in rgdayState
    int     cds;                 // number of months stored in rgdayState
    MONTHDAYSTATE   rgdayState[CALMONTHMAX + 2];

    int     nMonthDelta;        // the amount to move on button press

    BOOL    fControl;
    BOOL    fShift;
    
    CALENDARTYPE ct;            // information about the calendar

    WORD    fFocus:1;
    WORD    fEnabled:1;
    WORD    fCapture:1;         // mouse captured

    WORD    fSpinPrev:1;
    WORD    fFocusDrawn:1;      // is focus rect currently drawn?
    WORD    fToday:1;           // today's date currently visible in calendar
    WORD    fNoNotify:1;        // don't notify parent window
    WORD    fMultiSelecting:1;  // Are we actually in the process of selecting?
    WORD    fForwardSelect:1;
    WORD    fFirstDowSet:1;
    WORD    fTodaySet:1;
    WORD    fMinYrSet:1;        // stMin has been set
    WORD    fMaxYrSet:1;        // stMax has been set
    WORD    fMonthDelta:1;      // nMonthDelta has been set
    WORD    fHeaderRTL:1;       // Is header string RTL ?

    //
    //  Metrics for each month we display.
    //
    MONTHMETRICS rgmm[CALMONTHMAX];

} MONTHCAL, * PMONTHCAL, *LPMONTHCAL;


#define MonthCal_GetPtr(hwnd)      (MONTHCAL*)GetWindowPtr(hwnd, 0)
#define MonthCal_SetPtr(hwnd, p)   (MONTHCAL*)SetWindowPtr(hwnd, 0, p)

#define MonthCal_IsMultiSelect(pmc)     ((pmc)->ci.style & MCS_MULTISELECT)
#define MonthCal_IsDayState(pmc)        ((pmc)->ci.style & MCS_DAYSTATE)
#define MonthCal_ShowWeekNumbers(pmc)   ((pmc)->ci.style & MCS_WEEKNUMBERS)
#define MonthCal_ShowTodayCircle(pmc)   (!((pmc)->ci.style & MCS_NOTODAYCIRCLE))
#define MonthCal_ShowToday(pmc)         (!((pmc)->ci.style & MCS_NOTODAY))


//
// DATEPICK stuff
//

#define DPYBORDER       2
#define DPXBUFFER       2
#define DP_DXBUTTON     15
#define DP_DYBUTTON     15
#define DP_IDAUTOSPIN   1
#define DP_MSECAUTOSPIN 200
#define DATEPICK_UPDOWN 1000

#define DTP_FORMATLENGTH 128

enum {
    DP_SEL_DOW = 0,
    DP_SEL_YEAR,
    DP_SEL_MONTH,
    DP_SEL_DAY,
    DP_SEL_SEP1,
    DP_SEL_SEP2,
    DP_SEL_NODATE,
    DP_SEL_MAX
};

typedef struct tagDATEPICK {
    CONTROLINFO ci;     // all controls start with this

    HWND        hwndUD;
    HWND        hwndMC;
    HFONT       hfontMC;    // font for drop down cal

    COLORREF clr[MCSC_COLORCOUNT];

    // HACK! stMin and stMax must remain in order and adjacent
    SYSTEMTIME  stMin;      // minimum date we allow
    SYSTEMTIME  stMax;      // maximum date we allow
    SYSTEMTIME  stPrev;     // most recent date notified
    SUBEDITCONTROL sec;     // current date

    RECT        rcCheck;    // location of checkbox iff fShowNone
    RECT        rc;         // size of SEC space
    RECT        rcBtn;      // location of dropdown or updown
    int         iseLastActive; // which subedit was active when we were last active?
    WPARAM      gdtr;       // Did app set min and/or max? (GDTR_MIN|GDTR_MAX)

    BITBOOL         fEnabled:1;
    BITBOOL         fUseUpDown:1;
    BITBOOL         fFocus:1;
    BITBOOL         fNoNotify:1;
    BITBOOL         fCapture:1;
    BITBOOL         fShow:1;        // TRUE iff we should continue to show MonthCal

    BITBOOL         fCheck:1;       // TRUE iff the checkbox is checked
    BITBOOL         fCheckFocus:1;  // TRUE iff the checkbox has focus

    BITBOOL         fLocale:1;      // TRUE iff the format string is LOCALE dependent
    BITBOOL         fHasMark:1;      // true iff has am/pm in current format
    BITBOOL         fFreeEditing:1; // TRUE if in the middle of a free-format edit
} DATEPICK, * PDATEPICK, *LPDATEPICK;

#define DatePick_ShowCheck(pdp)     ((pdp)->ci.style & DTS_SHOWNONE)
#define DatePick_AppCanParse(pdp)   ((pdp)->ci.style & DTS_APPCANPARSE)
#define DatePick_RightAlign(pdp)    ((pdp)->ci.style & DTS_RIGHTALIGN)

#define DatePick_GetPtr(hwnd)      (DATEPICK*)GetWindowPtr(hwnd, 0)
#define DatePick_SetPtr(hwnd, p)   (DATEPICK*)SetWindowPtr(hwnd, 0, p)

#define CopyDate(stS, stD)  ((stD).wYear = (stS).wYear,(stD).wMonth = (stS).wMonth,(stD).wDay = (stS).wDay)
#define CopyTime(stS, stD)  ((stD).wHour = (stS).wHour,(stD).wMinute = (stS).wMinute,(stD).wSecond = (stS).wSecond)
