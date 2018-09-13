#include "ctlspriv.h"

#include "scdttime.h"
#include "monthcal.h"
#include "prshti.h"         // for StrDup_AtoW

// TODO
//
// #6329: When Min/Max range is set, then dates before the min
// or after the max are painted in the normal date color. They
// should be painted with MCSC_TRAILINGTEXT color. (Or we should
// add a new color to cover this case.) Feature requested by Jobi George
//
// 9577: We want a DAYSTATE like structure for the background
// color of dates. For highlighting. Perhaps a COLORSTATE per
// registered background color.
//

// private message
#define MCMP_WINDOWPOSCHANGED (MCM_FIRST - 1) // MCM_FIRST is way over WM_USER
#define DTMP_WINDOWPOSCHANGED (DTM_FIRST - 1) // DTM_FIRST is way over WM_USER

// MONTHCAL
LRESULT CALLBACK MonthCalWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT MCNcCreateHandler(HWND hwnd);
LRESULT MCCreateHandler(MONTHCAL *pmc, HWND hwnd, LPCREATESTRUCT lpcs);
LRESULT MCOnStyleChanging(MONTHCAL *pmc, UINT gwl, LPSTYLESTRUCT pinfo);
LRESULT MCOnStyleChanged(MONTHCAL *pmc, UINT gwl, LPSTYLESTRUCT pinfo);
void MCCalcSizes(MONTHCAL *pmc);
void MCHandleSetFont(MONTHCAL *pmc, HFONT hfont, BOOL fRedraw);
void MCPaint(MONTHCAL *pmc, HDC hdc);
void MCPaintMonth(MONTHCAL *pmc, HDC hdc, RECT *prc, int iMonth, int iYear, int iIndex,
                  BOOL fDrawPrev, BOOL fDrawNext, HBRUSH hbrSelect);
void MCNcDestroyHandler(HWND hwnd, MONTHCAL *pmc, WPARAM wParam, LPARAM lParam);
void MCRecomputeSizing(MONTHCAL *pmc, RECT *prect);
LRESULT MCSizeHandler(MONTHCAL *pmc, RECT *prc);
void MCUpdateMonthNamePos(MONTHCAL *pmc);
void MCUpdateStartEndDates(MONTHCAL *pmc, SYSTEMTIME *pstStart);
void MCGetRcForDay(MONTHCAL *pmc, int iMonth, int iDay, RECT *prc);
void MCGetRcForMonth(MONTHCAL *pmc, int iMonth, RECT *prc);
void MCUpdateToday(MONTHCAL *pmc);
void MCUpdateRcDayCur(MONTHCAL *pmc, SYSTEMTIME *pst);
void MCUpdateDayState(MONTHCAL *pmc);
int MCGetOffsetForYrMo(MONTHCAL *pmc, int iYear, int iMonth);
int MCIsSelectedDayMoYr(MONTHCAL *pmc, int iDay, int iMonth, int iYear);
BOOL MCIsBoldOffsetDay(MONTHCAL *pmc, int nDay, int iIndex);
BOOL FGetOffsetForPt(MONTHCAL *pmc, POINT pt, int *piOffset);
BOOL FGetRowColForRelPt(MONTHCAL *pmc, POINT ptRel, int *piRow, int *piCol);
BOOL FGetDateForPt(MONTHCAL *pmc, POINT pt, SYSTEMTIME *pst, 
                   int* piDay, int* piCol, int* piRow, LPRECT prcMonth);
LRESULT MCContextMenu(MONTHCAL *pmc, WPARAM wParam, LPARAM lParam);
LRESULT MCLButtonDown(MONTHCAL *pmc, WPARAM wParam, LPARAM lParam);
LRESULT MCLButtonUp(MONTHCAL *pmc, WPARAM wParam, LPARAM lParam);
LRESULT MCMouseMove(MONTHCAL *pmc, WPARAM wParam, LPARAM lParam);
LRESULT MCHandleTimer(MONTHCAL *pmc, WPARAM wParam);
LRESULT MCHandleKeydown(MONTHCAL *pmc, WPARAM wParam, LPARAM lParam);
LRESULT MCHandleChar(MONTHCAL *pmc, WPARAM wParam, LPARAM lParam);
int MCIncrStartMonth(MONTHCAL *pmc, int nDelta, BOOL fDelayDayChange);
void MCGetTitleRcsForOffset(MONTHCAL* pmc, int iOffset, LPRECT prcMonth, LPRECT prcYear);
BOOL MCSetDate(MONTHCAL *pmc, SYSTEMTIME *pst);
void MCNotifySelChange(MONTHCAL *pmc, UINT uMsg);
void MCInvalidateDates(MONTHCAL *pmc, SYSTEMTIME *pst1, SYSTEMTIME *pst2);
void MCInvalidateMonthDays(MONTHCAL *pmc);
void MCSetToday(MONTHCAL* pmc, SYSTEMTIME* pst);
void MCGetTodayBtnRect(MONTHCAL *pmc, RECT *prc);
void GetYrMoForOffset(MONTHCAL *pmc, int iOffset, int *piYear, int *piMonth);
BOOL FScrollIntoView(MONTHCAL *pmc);
void MCFreeCalendarInfo(PCALENDARTYPE pct);
void MCGetCalendarInfo(PCALENDARTYPE pct);
BOOL MCIsDateStringRTL(TCHAR tch);

// DATEPICK
LRESULT CALLBACK DatePickWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT DPNcCreateHandler(HWND hwnd);
LRESULT DPCreateHandler(DATEPICK *pdp, HWND hwnd, LPCREATESTRUCT lpcs);
LRESULT DPOnStyleChanging(DATEPICK *pdp, UINT gwl, LPSTYLESTRUCT pinfo);
LRESULT DPOnStyleChanged(DATEPICK *pdp, UINT gwl, LPSTYLESTRUCT pinfo);
void DPHandleLocaleChange(DATEPICK *pdp);
void DPDestroyHandler(HWND hwnd, DATEPICK *pdp, WPARAM wParam, LPARAM lParam);
void DPHandleSetFont(DATEPICK *pdp, HFONT hfont, BOOL fRedraw);
void DPPaint(DATEPICK *pdp, HDC hdc);
void DPLBD_MonthCal(DATEPICK *pdp, BOOL fLButtonDown);
LRESULT DPLButtonDown(DATEPICK *pdp, WPARAM wParam, LPARAM lParam);
LRESULT DPLButtonUp(DATEPICK *pdp, WPARAM wParam, LPARAM lParam);
void DPRecomputeSizing(DATEPICK *pdp, RECT *prect);
LRESULT DPHandleKeydown(DATEPICK *pdp, WPARAM wParam, LPARAM lParam);
LRESULT DPHandleChar(DATEPICK *pdp, WPARAM wParam, LPARAM lParam);
void DPNotifyDateChange(DATEPICK *pdp);
BOOL DPSetDate(DATEPICK *pdp, SYSTEMTIME *pst, BOOL fMungeDate);
void DPDrawDropdownButton(DATEPICK *pdp, HDC hdc, BOOL fPressed);
void SECGetSystemtime(LPSUBEDITCONTROL psec, LPSYSTEMTIME pst);

static TCHAR const g_rgchMCName[] = MONTHCAL_CLASS;
static TCHAR const g_rgchDTPName[] = DATETIMEPICK_CLASS;

// MONTHCAL globals
#define g_szTextExtentDef TEXT("0000")
#define g_szNumFmt TEXT("%d")

//
//  Epoch = the beginning of the universe (the earliest date we support)
//  Armageddon = the end of the universe (the latest date we support)
//
//  Epoch is 14-sep-1752 because that's when the Gregorian calendar
//  kicked in.  The day before 14-sep-1752 was 2-sep-1752 (in British
//  and US history; other countries switched at other times).
//
//  Armageddon is 31-dec-9999 because we assume four digits for years
//  is enough.  (Oh no, the Y10K problem...)
//
const SYSTEMTIME c_stEpoch      = { 1752,  9, 0, 14,  0,  0,  0,   0 };
const SYSTEMTIME c_stArmageddon = { 9999, 12, 0, 31, 23, 59, 59, 999 };

void FillRectClr(HDC hdc, LPRECT prc, COLORREF clr)
{
    COLORREF clrSave = SetBkColor(hdc, clr);
    ExtTextOut(hdc,0,0,ETO_OPAQUE,prc,NULL,0,NULL);
    SetBkColor(hdc, clrSave);
}

BOOL InitDateClasses(HINSTANCE hinst)
{
    WNDCLASS wndclass;
    
    if (GetClassInfo(hinst, g_rgchMCName, &wndclass))
    {
        // we're already registered
        DebugMsg(TF_MONTHCAL, TEXT("mc: Date Classes already initialized."));
        return(TRUE);
    }
    
    wndclass.style          = CS_GLOBALCLASS;
    wndclass.lpfnWndProc    = (WNDPROC)MonthCalWndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = sizeof(LPVOID);
    wndclass.hInstance      = hinst;
    wndclass.hIcon          = NULL;
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = g_rgchMCName;

    if (!RegisterClass(&wndclass))
    {
        DebugMsg(DM_WARNING, TEXT("mc: MonthCalClass failed to initialize"));
        return(FALSE);
    }

    wndclass.lpfnWndProc    = (WNDPROC)DatePickWndProc;
    wndclass.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    wndclass.lpszClassName  = g_rgchDTPName;

    if (!RegisterClass(&wndclass))
    {
        DebugMsg(DM_WARNING, TEXT("mc: DatePickClass failed to initialize"));
        return(FALSE);
    }

    DebugMsg(TF_MONTHCAL, TEXT("mc: Date Classes initialized successfully."));
    return(TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
// MonthCal stuff
//
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////
//
// MCInsert/RemoveMarkers
//
// QuickSummary:  Convert the string "MMMM yyyy" into "\1MMMM\3 \2yyyy\4".
//
// In order to lay out the month/year info in the header, we have to be
// able to extract the month and year out of the formatted string so we
// know what their rectangles are.  We do this by wrapping the month and
// year inserts with markers so we can extract them after formatting.
//
// Since \1 through \4 are control characters, they won't conflict with
// displayable characters in the actual format string.  And just to play it
// safe, if we actually see a format character, we erase it from the string.
//
// MCInsertMarkers inserts the markers into the output string so we can
// extract the substrings later.  Quotation marks are funky since you can
// write a format of "'The' mm'''th month of' yyyy".  Note that a simple
// even-odd test works for detecting whether we are inside or outside
// quotation marks, even in the nested quotation mark case.
//
void MCInsertMarkers(LPTSTR pszOut, LPCTSTR pszIn)
{
    BOOL fInQuote = FALSE;
    UINT flSeen = 0;
    UINT flThis;

    for (;;)
    {
        TCHAR ch = *pszIn;
        switch (ch) {

        // At end of string, terminate the output buffer and go home
        case TEXT('\0'):
            *pszOut = TEXT('\0');
            return;

        case TEXT('m'):
        case TEXT('M'):
            flThis = IMM_MONTHSTART;
            goto CheckMarker;

        case TEXT('y'):
            flThis = IMM_YEARSTART;
            goto CheckMarker;

        CheckMarker:
            // If inside a quotation mark or we've already done this guy,
            // then just treat it as a regular character.
            if (fInQuote || (flSeen & flThis))
                goto CopyChar;

            flSeen |= flThis;

            *pszOut++ = (TCHAR)flThis;
            // Don't need to use CharNext because we know *pszIn is "m" "M" or "y"
            for ( ; *pszIn == ch; pszIn++)
            {
                *pszOut++ = ch;
            }
            *pszOut++ = (TCHAR)(flThis + DMM_STARTEND);

            // Restart the loop so we re-parse the character at *pszIn
            continue;

        // Toggle the quotation mark gizmo if we see one, and then just
        // copy it.
        case '\'':
            fInQuote ^= TRUE;
            goto CopyChar;

        //
        //  Don't let these sneak into the output format or it
        //  will confuse us.
        //
        case IMM_MONTHSTART:
        case IMM_YEARSTART:
        case IMM_MONTHEND:
        case IMM_YEAREND:
            break;

        default:
        CopyChar:
            *pszOut++ = ch;
#ifndef UNICODE
            if (IsDBCSLeadByte(ch) && pszIn[1]) {
                *pszOut++ = *++pszIn;
            }
#endif
            break;

        }

        pszIn++;            // We handled the DBCS case already
    }

    // NOTREACHED
}

//
//  MCRemoveMarkers hunts down the marker characters and strips them out,
//  recording their locations in the optional MONTHMETRICS (as character
//  indices).
//

void MCRemoveMarkers(LPTSTR pszBuf, PMONTHMETRICS pmm)
{
    int iWrite, iRead;

    //
    //  If by some horrid error we can't find our markers, just pretend
    //  they were at the start of the string.
    //
    if (pmm) {
        pmm->rgi[IMM_MONTHSTART] = 0;
        pmm->rgi[IMM_YEARSTART ] = 0;
        pmm->rgi[IMM_MONTHEND  ] = 0;
        pmm->rgi[IMM_YEAREND   ] = 0;
    }

    iWrite = iRead = 0;
    for (;;)
    {
        TCHAR ch = pszBuf[iRead];
        switch (ch)
        {
        // At end of string, terminate the output buffer and go home
        case TEXT('\0'):
            pszBuf[iWrite] = TEXT('\0');
            return;

        // If we find a marker, eat it and remember its location
        case IMM_MONTHSTART:
        case IMM_YEARSTART:
        case IMM_MONTHEND:
        case IMM_YEAREND:
            if (pmm)
                pmm->rgi[ch] = iWrite;
            break;

        // Otherwise, just copy it to the output
        default:
            pszBuf[iWrite++] = ch;
#ifndef UNICODE
            if (IsDBCSLeadByte(ch) && pszBuf[iRead+1]) {
                pszBuf[iWrite++] = pszBuf[++iRead];
            }
#endif
            break;

        }
        iRead++;
    }
    // NOTREACHED
}

////////////////////////////////////
//
// Like LocalizedLoadString, except that we get the string from
// LOCAL_USER_DEFAULT instead of GetUserDefaultUILanguage().
//
// LOCALE_USER_DEFAULT is the same as GetUserDefaultLCID(), and
// LANGIDFROMLCID(GetUserDefaultLCID()) is the same as GetUserDefaultLangID().
//
// So we pass GetUserDefaultLangID() as the language.
//

int MCLoadString(UINT uID, LPWSTR lpBuffer, int nBufferMax)
{
    return CCLoadStringEx(uID, lpBuffer, nBufferMax, GetUserDefaultLangID());
}

////////////////////////////////////
//
// Get the localized calendar info
//
BOOL UpdateLocaleInfo(MONTHCAL* pmc, LPLOCALEINFO pli)
{
    int    i;
    TCHAR  szBuf[64];
    int    cch;
    LPTSTR pc = szBuf;


    //
    // Get information about the calendar (e.g., is it supported?)
    //
    MCGetCalendarInfo(&pmc->ct);

    //
    // Check if the calendar title is an RTL string
    //
    GetDateFormat(pmc->ct.lcid, 0, NULL, TEXT("MMMM"), szBuf, ARRAYSIZE(szBuf));
    pmc->fHeaderRTL = (WORD) MCIsDateStringRTL(szBuf[0]);

    //
    // get the short date format and sniff it to see if it displays the year
    // or month first
    //
    MCLoadString(IDS_MONTHFMT, pli->szMonthFmt, ARRAYSIZE(pli->szMonthFmt));

    //
    //  Try to get the MONTHYEAR format from NLS.  If not supported by NLS,
    //  then use the hard-coded value in our resources.  Note that we
    //  subtract 4 from the buffer size because we may insert up to four
    //  marker characters.
    //
    COMPILETIME_ASSERT(ARRAYSIZE(szBuf) >= ARRAYSIZE(pli->szMonthYearFmt));
    szBuf[0] = TEXT('\0');

    GetLocaleInfo(pmc->ct.lcid, LOCALE_SYEARMONTH,
                 szBuf, ARRAYSIZE(pli->szMonthYearFmt) - CCH_MARKERS);
    if (!szBuf[0]) {
        MCLoadString(IDS_MONTHYEARFMT, szBuf, ARRAYSIZE(pli->szMonthYearFmt) - CCH_MARKERS);
    }

    MCInsertMarkers(pli->szMonthYearFmt, szBuf);

    //
    //  BUGBUG - this code needs to change to use CAL_ values when we
    //  want to support multiple calendars.
    //

    //
    // Get the month names
    //
    for (i = 0; i < 12; i++)
    {
        cch = GetLocaleInfo(pmc->ct.lcid, LOCALE_SMONTHNAME1 + i,
                            pli->rgszMonth[i], CCHMAXMONTH);
        if (cch == 0)
            // the calendar is pretty useless without month names...
            return(FALSE);
    }

    //
    // Get the days of the week
    //
    for (i = 0; i < 7; i++)
    {
        cch = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVDAYNAME1 + i,
                            pli->rgszDay[i], CCHMAXABBREVDAY);
        if (cch == 0)
            // the calendar is pretty useless without day names...
            return(FALSE);
    }

    //
    // If we haven't already set what the first day of the week is, get the
    // localized setting.
    // 
    if (!pmc->fFirstDowSet)
    {
        cch = GetLocaleInfo(pmc->ct.lcid, LOCALE_IFIRSTDAYOFWEEK, szBuf, ARRAYSIZE(szBuf));
        if (cch > 0)
            pli->dowStartWeek = szBuf[0] - TEXT('0');
    }

    //
    // Get the first week of the year
    //
    cch = GetLocaleInfo(pmc->ct.lcid, LOCALE_IFIRSTWEEKOFYEAR, szBuf, ARRAYSIZE(szBuf));
    if (cch > 0)
        pli->firstWeek = szBuf[0] - TEXT('0');
    
    // Set up pointers
    for (i = 0; i < 12; i++)
        pli->rgpszMonth[i] = pli->rgszMonth[i];

    for (i = 0; i < 7; i++)
        pli->rgpszDay[i] = pli->rgszDay[i];

    // Get static strings
    MCLoadString(IDS_TODAY, pli->szToday, ARRAYSIZE(pli->szToday));
    MCLoadString(IDS_GOTOTODAY, pli->szGoToToday, ARRAYSIZE(pli->szGoToToday));

    // if we've been initialized
    if (pmc->hinstance)
    {
        SYSTEMTIME st;
        CopyDate(pmc->stMonthFirst, st);
        MCUpdateStartEndDates(pmc, &st);
    }
    return(TRUE);
}

void MCReloadMenus(MONTHCAL *pmc)
{
    int i;

    if (pmc->hmenuCtxt)
        DestroyMenu(pmc->hmenuCtxt);
    if (pmc->hmenuMonth)
        DestroyMenu(pmc->hmenuMonth);

    pmc->hmenuCtxt = CreatePopupMenu();
    if (pmc->hmenuCtxt)
        AppendMenu(pmc->hmenuCtxt, MF_STRING, 1, pmc->li.szGoToToday);

    pmc->hmenuMonth = CreatePopupMenu();
    if (pmc->hmenuMonth)
    {
        for (i = 0; i < 12; i++)
            AppendMenu(pmc->hmenuMonth, MF_STRING, i + 1, pmc->li.rgszMonth[i]);
    }
}

BOOL MCHandleEraseBkgnd(MONTHCAL* pmc, HDC hdc)
{
    RECT rc;

    GetClipBox(hdc, &rc);
    FillRectClr(hdc, &rc, pmc->clr[MCSC_BACKGROUND]);
    return TRUE;   
}


LRESULT MCHandleHitTest(MONTHCAL* pmc, PMCHITTESTINFO phti)
{
    int iMonth;
    RECT rc;
    
    if (!phti || phti->cbSize != sizeof(MCHITTESTINFO))
        return -1;

    phti->uHit = MCHT_NOWHERE;

    MCGetTodayBtnRect(pmc, &rc);
    if (PtInRect(&rc, phti->pt) && MonthCal_ShowToday(pmc))
    {
        phti->uHit = MCHT_TODAYLINK;
    }
    else if (pmc->fSpinPrev = (WORD) PtInRect(&pmc->rcPrev, phti->pt))
    {
        phti->uHit = MCHT_TITLEBTNPREV;        
    }
    else if (PtInRect(&pmc->rcNext, phti->pt))
    {
        phti->uHit = MCHT_TITLEBTNNEXT;        
    }
    else if (FGetOffsetForPt(pmc, phti->pt, &iMonth))
    {
        RECT  rcMonth;   // bounding rect for month containg phti->pt
        POINT ptRel;     // relative point in a month
        int   month;
        int   year;

        MCGetRcForMonth(pmc, iMonth, &rcMonth);
        ptRel.x = phti->pt.x - rcMonth.left;
        ptRel.y = phti->pt.y - rcMonth.top;

        GetYrMoForOffset(pmc, iMonth, &year, &month);
        phti->st.wMonth = (WORD) month;
        phti->st.wYear  = (WORD) year;
            
        // 
        // if calendar is showing week numbers and the point lies in the
        // the week numbers, get the date for day immediately to the right
        // of the week number containing the point
        //
        if (MonthCal_ShowWeekNumbers(pmc) && PtInRect(&pmc->rcWeekNum, ptRel))
        {            
            phti->uHit |= MCHT_CALENDARWEEKNUM;
            phti->pt.x += pmc->rcDayNum.left;
            FGetDateForPt(pmc, phti->pt, &phti->st, NULL, NULL, NULL, NULL);
        }

        //
        // if the point lies in the days of the week header, then return
        // the day of the week containing the point
        //
        else if (PtInRect(&pmc->rcDow, ptRel))
        {            
            int iRow;
            int iCol;
            
            phti->uHit |= MCHT_CALENDARDAY;
            ptRel.y = pmc->rcDayNum.top;
            FGetRowColForRelPt(pmc, ptRel, &iRow, &iCol);
            phti->st.wDayOfWeek = (WORD) iCol;            
        }

        //
        // if the point lies in the actually calendar part, then return the
        // date containg the point
        //
        else if (PtInRect(&pmc->rcDayNum, ptRel))
        {
            int iDay;
            
            // we're in the calendar part!
            phti->uHit |= MCHT_CALENDAR;

            if (FGetDateForPt(pmc, phti->pt, &phti->st, &iDay, NULL, NULL, NULL))
            {                
                phti->uHit |= MCHT_CALENDARDATE;
                
                // if it was beyond the bounds of the days we're showing
                // and also FGetDateForPt returns TRUE, then we're on the boundary
                // of the displayed months
                if (iDay <= 0)
                {
                    phti->uHit |= MCHT_PREV;
                }
                else if (iDay > pmc->rgcDay[iMonth + 1])
                {
                    phti->uHit |= MCHT_NEXT;
                }
            }            
        }
        else
        {
            RECT rcMonthTitle;
            RECT rcYearTitle;

            // otherwise we're in the title
            
            phti->uHit |= MCHT_TITLE;
            MCGetTitleRcsForOffset(pmc, iMonth, &rcMonthTitle, &rcYearTitle);

            if (PtInRect(&rcMonthTitle, phti->pt))
            {
                phti->uHit |= MCHT_TITLEMONTH;
            }
            else if (PtInRect(&rcYearTitle, phti->pt))
            {
                phti->uHit |= MCHT_TITLEYEAR;
            }
        }
    }    

    DebugMsg(TF_MONTHCAL, TEXT("mc: Hittest returns : %d %d %d %d)"), 
             (int)phti->st.wDay,
             (int)phti->st.wMonth, 
             (int)phti->st.wYear,
             (int)phti->st.wDayOfWeek
             );
    
    return phti->uHit;
}

void MonthCal_OnPaint(MONTHCAL *pmc, HDC hdc)
{
    if (hdc)
    {
        MCPaint(pmc, hdc);
    }
    else
    {
        PAINTSTRUCT ps;
        hdc = BeginPaint(pmc->ci.hwnd, &ps);
        MCPaint(pmc, hdc);
        EndPaint(pmc->ci.hwnd, &ps);
    }
}

BOOL MCGetDateFormatWithTempYear(PCALENDARTYPE pct, SYSTEMTIME *pst, LPCTSTR pszFormat, UINT uYear, LPTSTR pszBuf, UINT cchBuf)
{
    BOOL fRc;
    WORD wYear = pst->wYear;
    pst->wYear = (WORD)uYear;
    fRc = GetDateFormat(pct->lcid, 0, pst, pszFormat, pszBuf, cchBuf);
    if (!fRc)
    {
        // AIGH!  I hate Feburary 29.  In case we are Feb 29 1996 and the
        // user changes to a non-leap year, force the day to something valid
        // in February 1997 (or whatever year the user finally picked).
        //
        // We can't blindly smash the day to 1 because the era might change
        // in the middle of the month.
        WORD wDay = pst->wDay;

        ASSERT(pst->wDay == 29);
        pst->wDay = 28;
        fRc = GetDateFormat(pct->lcid, 0, pst, pszFormat, pszBuf, cchBuf);
        pst->wDay = wDay;
    }
    pst->wYear = wYear;
    return fRc;
}

void MCUpdateEditYear(MONTHCAL *pmc)
{
    TCHAR rgch[64];

    ASSERT(pmc->hwndEdit);

    EVAL(MCGetDateFormatWithTempYear(&pmc->ct, &pmc->st, TEXT("yyyy"), pmc->st.wYear, rgch, ARRAYSIZE(rgch)));

    SendMessage(pmc->hwndEdit, WM_SETTEXT, 0, (LPARAM)rgch);
}


LRESULT CALLBACK MonthCalWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MONTHCAL *pmc;
    LRESULT lres = 0;
        
    if (uMsg == WM_NCCREATE)
        return(MCNcCreateHandler(hwnd));
    
    pmc = MonthCal_GetPtr(hwnd);
    if (pmc == NULL)
        return(DefWindowProc(hwnd, uMsg, wParam, lParam));

    // Dispatch the various messages we can receive
    switch (uMsg)
    {
        
    case WM_CREATE:
        lres = MCCreateHandler(pmc, hwnd, (LPCREATESTRUCT)lParam);
        break;
        
        HANDLE_MSG(pmc, WM_ERASEBKGND, MCHandleEraseBkgnd);

    case WM_PRINTCLIENT:
    case WM_PAINT:
        MonthCal_OnPaint(pmc, (HDC)wParam);
        return(0);

    case WM_KEYDOWN:
        MCHandleKeydown(pmc, wParam, lParam);
        break;

    case WM_KEYUP:
        switch (wParam)
        {   
        case VK_CONTROL:
            pmc->fControl = FALSE;
            break;

        case VK_SHIFT:
            pmc->fShift = FALSE;
            break;
        }
        break;    

        
#if 0
    case WM_CHAR:
        MCHandleChar(pmc, wParam, lParam);
        break;
#endif
                
    case WM_CONTEXTMENU:
        MCContextMenu(pmc, wParam, lParam);
        break;

    case WM_LBUTTONDOWN:
        MCLButtonDown(pmc, wParam, lParam);
        break;

    case WM_LBUTTONUP:
        MCLButtonUp(pmc, wParam, lParam);
        break;

    case WM_MOUSEMOVE:
        MCMouseMove(pmc, wParam, lParam);
        break;

    case WM_GETFONT:
        lres = (LRESULT)pmc->hfont;
        break;

    case WM_SETFONT:
        MCHandleSetFont(pmc, (HFONT)wParam, (BOOL)LOWORD(lParam));
        MCSizeHandler(pmc, &pmc->rc);
        MCUpdateMonthNamePos(pmc);
        break;

    case WM_TIMER:
        MCHandleTimer(pmc, wParam);
        break;

    case WM_NCDESTROY:
        MCNcDestroyHandler(hwnd, pmc, wParam, lParam);
        break;

    case WM_ENABLE:
    {
        BOOL fEnable = wParam ? TRUE:FALSE;
        if (pmc->fEnabled != fEnable)
        {
            pmc->fEnabled = (WORD) fEnable;
            InvalidateRect(pmc->ci.hwnd, NULL, TRUE);
        }
        break;
    }

    case MCMP_WINDOWPOSCHANGED:
    case WM_SIZE:
    {
        RECT rc;

        if (uMsg==MCMP_WINDOWPOSCHANGED)
        {
            GetClientRect(pmc->ci.hwnd, &rc);
        }
        else
        {
            rc.left   = 0;
            rc.top    = 0;
            rc.right  = GET_X_LPARAM(lParam);
            rc.bottom = GET_Y_LPARAM(lParam);
        }

        lres = MCSizeHandler(pmc, &rc);
        break;
    }

    case WM_CANCELMODE:
        PostMessage(pmc->ci.hwnd, WM_LBUTTONUP, 0, 0xFFFFFFFF);
        break;

    case WM_SYSCOLORCHANGE:
        InitGlobalColors();
        break;

    case WM_WININICHANGE:
        InitGlobalMetrics(wParam);

        if (lParam == 0 ||
#ifdef UNICODE_WIN9x
            !lstrcmpiA((LPSTR)lParam, "Intl")
#else
            !lstrcmpi((LPTSTR)lParam, TEXT("Intl"))
#endif
           )
        {
            UpdateLocaleInfo(pmc, &pmc->li);
            MCReloadMenus(pmc);
            InvalidateRect(hwnd, NULL, TRUE);
            wParam = 0;             // force MCCalcSizes to happen
        }
        if (wParam == 0 || wParam == SPI_SETNONCLIENTMETRICS)
        {
            MCCalcSizes(pmc);
            PostMessage(pmc->ci.hwnd, MCMP_WINDOWPOSCHANGED, 0, 0);
        }

        break;

    case WM_NOTIFYFORMAT:
        return CIHandleNotifyFormat(&pmc->ci, lParam);
        break;

    case WM_STYLECHANGING:
        lres = MCOnStyleChanging(pmc, (UINT) wParam, (LPSTYLESTRUCT)lParam);
        break;

    case WM_STYLECHANGED:
        lres = MCOnStyleChanged(pmc, (UINT) wParam, (LPSTYLESTRUCT)lParam);
        break;

    case WM_NOTIFY: {
        LPNMHDR pnm = (LPNMHDR)lParam;
        switch (pnm->code)
        {
        case UDN_DELTAPOS:
            if (pnm->hwndFrom == pmc->hwndUD)
            {
                // A notification from the UpDown control buddied
                // with the currently popped up monthcal, adjust the
                // edit box appropriately.  We use UDN_DELTAPOS instad
                // of WM_VSCROLL because we care only about the delta and
                // not the absolute number. The absolute number causes us
                // problems in localized calendars.
                LPNM_UPDOWN pnmdp = (LPNM_UPDOWN)lParam;
                UINT yr = pmc->st.wYear + pnmdp->iDelta;
                UINT yrMin, yrMax;
                int delta;

                yrMin = pmc->stMin.wYear;
                if (yr < yrMin)
                    yr = yrMin;

                yrMax = pmc->stMax.wYear;
                if (yr > yrMax)
                    yr = yrMax;

                delta = yr - pmc->st.wYear;
                pmc->st.wYear = (WORD)yr;
                if (delta) {
                    MCIncrStartMonth(pmc, delta * 12, FALSE);
                    MCNotifySelChange(pmc,MCN_SELCHANGE);

                }
            }
            break;
        }
    } // WM_NOTIFY switch
        break;

    case WM_VSCROLL:
        // this must be coming from our UpDown control buddied
        // with the currently popped up monthcal, adjust the
        // edit box appropriately
        // We must do this on WM_VSCROLL rather than UDN_DELTAPOS
        // since we need to fix the selection after the updown mangled it
        MCUpdateEditYear(pmc);
        break;


    //
    // MONTHCAL specific messages
    //


    // MCM_GETCURSEL wParam=void lParam=LPSYSTEMTIME
    //   sets *lParam to the currently selected SYSTEMTIME
    //   returns TRUE on success, FALSE on error (such as multi-select MONTHCAL)
    case MCM_GETCURSEL:
        if (!MonthCal_IsMultiSelect(pmc))
        {
            LPSYSTEMTIME pst = (LPSYSTEMTIME)lParam;
            if (pst)
            {
                ZeroMemory(pst, sizeof(SYSTEMTIME));
                // BUGBUG raymondc v6. Need to zero out the time fields instead of
                // setting them to garbage.  This confuses MFC.
                *pst = pmc->st;
                pst->wDayOfWeek = (DowFromDate(pst)+1) % 7;  // this returns 0==sun
                lres = 1;
            }
        }
        break;

    // MCM_SETCURSEL wParam=void lParam=LPSYSTEMTIME
    //   sets the currently selected SYSTEMTIME to *lParam
    //   returns TRUE on success, FALSE on error (such as multi-select MONTHCAL or bad parameters)
    case MCM_SETCURSEL:
    {
        LPSYSTEMTIME pst = (LPSYSTEMTIME)lParam;

        if (MonthCal_IsMultiSelect(pmc) ||
            !IsValidDate(pst))
        {
            break;
        }

        if (0 == CmpDate(pst, &pmc->st))
        {
            // if no change, just return
            lres = 1;
            break;
        }

        pmc->rcDayOld = pmc->rcDayCur;

        pmc->fNoNotify = TRUE;
        lres = MCSetDate(pmc, pst);
        pmc->fNoNotify = FALSE;

        if (lres)
        {
            InvalidateRect(pmc->ci.hwnd, &pmc->rcDayOld, FALSE);     // erase old highlight
            InvalidateRect(pmc->ci.hwnd, &pmc->rcDayCur, FALSE);     // draw new highlight
        }

        UpdateWindow(pmc->ci.hwnd);
        break;
    }

    // MCM_GETMAXSELCOUNT wParam=void lParam=void
    //   returns the max number of selected days allowed
    case MCM_GETMAXSELCOUNT:
        lres = (LRESULT)(MonthCal_IsMultiSelect(pmc) ? pmc->cSelMax : 1);
        break;

    // MCM_SETMAXSELCOUNT wParam=int lParam=void
    //   sets the maximum selectable date range to wParam days
    //   returns TRUE on success, FALSE on error (such as single-select MONTHCAL)
    case MCM_SETMAXSELCOUNT:
        if (!MonthCal_IsMultiSelect(pmc) || (int)wParam < 1)
            break;

        pmc->cSelMax = (int)wParam;
        lres = 1;
        break;

    // MCM_GETSELRANGE wParam=void lParam=LPSYSTEMTIME[2]
    //   sets *lParam to the first date of the range, *(lParam+1) to the second date
    //   returns TRUE on success, FALSE otherwise (such as single-select MONTHCAL)
    case MCM_GETSELRANGE:
    {
        LPSYSTEMTIME pst;

        pst = (LPSYSTEMTIME)lParam;

        if (!pst)
            break;

        ZeroMemory(pst, sizeof(SYSTEMTIME) * 2);

        if (!MonthCal_IsMultiSelect(pmc))
            break;

        *pst = pmc->st;
        pst->wDayOfWeek = (DowFromDate(pst)+1) % 7;  // this returns 0==sun
        pst++;
        *pst = pmc->stEndSel;
        pst->wDayOfWeek = (DowFromDate(pst)+1) % 7;  // this returns 0==sun
        lres = 1;

        break;
    }

    // MCM_SETSELRANGE wParam=void lParam=LPSYSTEMTIME[2]
    //   sets the currently selected day range to *lparam to *(lParam+1)
    //   returns TRUE on success, FALSE otherwise (such as single-select MONTHCAL or bad params)
    case MCM_SETSELRANGE:
    {
        LPSYSTEMTIME pstStart = (LPSYSTEMTIME)lParam;
        LPSYSTEMTIME pstEnd = &pstStart[1];
        SYSTEMTIME stStart;
        SYSTEMTIME stEnd;

        if (!MonthCal_IsMultiSelect(pmc) ||
            !IsValidDate(pstStart) ||
            !IsValidDate(pstEnd))
            break;

        // IE3 shipped without validating the time portion of this message.
        // Make sure our stored systemtimes are always valid (so we will
        // always give out valid systemtime structs).
        //
        if (!IsValidTime(pstStart))
            CopyTime(pmc->st, *pstStart);
        if (!IsValidTime(pstEnd))
            CopyTime(pmc->stEndSel, *pstEnd);

        if (CmpDate(pstStart, pstEnd) > 0)
        {
            stEnd = *pstStart;
            stStart = *pstEnd;
            pstStart = &stStart;
            pstEnd = &stEnd;
        }

        if (CmpDate(pstStart, &pmc->stMin) < 0)
            break;

        if (CmpDate(pstEnd, &pmc->stMax) > 0)
            break;

        if (DaysBetweenDates(pstStart, pstEnd) >= pmc->cSelMax)
            break;


        if (0 == CmpDate(pstStart, &pmc->st) &&
            0 == CmpDate(pstEnd, &pmc->stEndSel))
        {
            // if no change, just return
            lres = 1;
            break;
        }

        pmc->stStartPrev = pmc->st;
        pmc->stEndPrev = pmc->stEndSel;

        pmc->fNoNotify = TRUE;

        lres = MCSetDate(pmc, pstEnd);
        if (lres)
        {
            pmc->st = *pstStart;
            pmc->stEndSel = *pstEnd;

            MCInvalidateDates(pmc, &pmc->stStartPrev, &pmc->stEndPrev);
            MCInvalidateDates(pmc, &pmc->st, &pmc->stEndSel);
            UpdateWindow(pmc->ci.hwnd);
        }

        pmc->fNoNotify = FALSE;

        break;
    }
    
    // MCM_GETMONTHRANGE wParam=GMR_flags lParam=LPSYSTEMTIME[2]
    // if GMR_VISIBLE, returns the range of selectable (non-grayed) displayed
    // days. if GMR_DAYSTATE, returns the range of every (incl grayed) days.
    // returns the number of months the above range spans.
    case MCM_GETMONTHRANGE:
    {
        LPSYSTEMTIME pst = (LPSYSTEMTIME)lParam;

        if (pst)
        {
            ZeroMemory(pst, 2 * sizeof(SYSTEMTIME));

            if (wParam == GMR_VISIBLE)
            {
                pst[0] = pmc->stMonthFirst;
                pst[1] = pmc->stMonthLast;
            }
            else if (wParam == GMR_DAYSTATE)
            {
                pst[0] = pmc->stViewFirst;
                pst[1] = pmc->stViewLast;
            }
        }

        lres = (LRESULT)pmc->nMonths;
        if (wParam == GMR_DAYSTATE)
            lres += 2;
        
        break;
    }

    // MCM_SETDAYSTATE wParam=int lParam=LPDAYSTATE
    // updates the MONTHCAL's DAYSTATE, only for MONTHCALs with DAYSTATE enabled
    // the range of months represented in the DAYSTATE array passed in lParam 
    // should match that of the MONTHCAL
    // wParam count of items in DAYSTATE array
    // lParam pointer to array of DAYSTATE items 
    // returns FALSE if not DAYSTATE enabled or if an error occurs, TRUE otherwise
    case MCM_SETDAYSTATE:
    {
        MONTHDAYSTATE *pmds = (MONTHDAYSTATE *)lParam;
        int i;

        if (!MonthCal_IsDayState(pmc) ||
            (int)wParam != (pmc->nMonths + 2))
            break;

        for (i = 0; i < (int)wParam; i++)
        {
            pmc->rgdayState[i] = *pmds;
            pmds++;
        }
        MCInvalidateMonthDays(pmc);
        lres = 1;

        break;
    }

    // MCM_GETMINREQRECT wParam=void lParam=LPRECT
    //   sets *lParam to the minimum size required to display one month in full.
    //   Note: this is dependent upon the currently selected font.
    //   Apps can take the returned size and double the width to get two calendars
    //   displayed.
    case MCM_GETMINREQRECT:
    {
        LPRECT prc = (LPRECT)lParam;

        prc->left   = 0;
        prc->top    = 0;
        prc->right  = pmc->dxMonth;
        prc->bottom = pmc->dyMonth;
        if (MonthCal_ShowToday(pmc))
        {
            prc->bottom += pmc->dyToday;
        }

        AdjustWindowRect(prc, pmc->ci.style, FALSE);

        // This is a bogus message, lParam should really be LPSIZE.
        // Make sure left and top are 0 (AdjustWindowRect will make these negative).
        prc->right  -= prc->left;
        prc->bottom -= prc->top;
        prc->left    = 0;
        prc->top     = 0;

        lres = 1;

        break;
    }

    // MCM_GETMAXTODAYWIDTH wParam=void lParam=LPDWORD
    //   sets *lParam to the width of the "today" string, so apps
    //   can figure out how big to make the calendar (max of MCM_GETMINREQRECT
    //   and MCM_GETMAXTODAYWIDTH).
    case MCM_GETMAXTODAYWIDTH:
    {
        RECT rc;

        rc.left = 0;
        rc.top = 0;
        rc.right = pmc->dxToday;
        rc.bottom = pmc->dyToday;

        AdjustWindowRect(&rc, pmc->ci.style, FALSE);

        lres = rc.right - rc.left;
        break;
    }

    case MCM_HITTEST:
        return MCHandleHitTest(pmc, (PMCHITTESTINFO)lParam);
        
    case MCM_SETCOLOR:

        if (wParam >= 0 && wParam < MCSC_COLORCOUNT) 
        {
            COLORREF clr = pmc->clr[wParam];
            pmc->clr[wParam] = (COLORREF)lParam;
            InvalidateRect(hwnd, NULL, wParam == MCSC_BACKGROUND);
            return clr;
        }
        return -1;
        
    case MCM_GETCOLOR:
        if (wParam >= 0 && wParam < MCSC_COLORCOUNT) 
            return pmc->clr[wParam];
        return -1;
        
    case MCM_SETFIRSTDAYOFWEEK:
    {
        lres = MAKELONG(pmc->li.dowStartWeek, (BOOL)pmc->fFirstDowSet);
        if (lParam == (LPARAM)-1) {
            pmc->fFirstDowSet = FALSE;
        } else if (lParam < 7) {
            pmc->fFirstDowSet = TRUE;
            pmc->li.dowStartWeek = (TCHAR)lParam;
        }
        UpdateLocaleInfo(pmc, &pmc->li);
        InvalidateRect(hwnd, NULL, FALSE);
        return lres;
    }
        
    case MCM_GETFIRSTDAYOFWEEK:
        return MAKELONG(pmc->li.dowStartWeek, (BOOL)pmc->fFirstDowSet);
        
    case MCM_SETTODAY:
        MCSetToday(pmc, (SYSTEMTIME*)lParam);
        break;
        
    case MCM_GETTODAY:
        if (lParam) {
            *((SYSTEMTIME*)lParam) = pmc->stToday;
            return TRUE;
        }
        return FALSE;

    case MCM_GETRANGE:
        if (lParam)
        {
            LPSYSTEMTIME pst = (LPSYSTEMTIME)lParam;

            ZeroMemory(pst, sizeof(SYSTEMTIME)*2);

            ASSERT(lres == 0);
            if (pmc->fMinYrSet)
            {
                pst[0] = pmc->stMin;
                lres = GDTR_MIN;
            }
            if (pmc->fMaxYrSet)
            {
                pst[1] = pmc->stMax;
                lres |= GDTR_MAX;
            }
        }
        break;

    case MCM_SETRANGE:
        if (lParam)
        {
            LPSYSTEMTIME pst = (LPSYSTEMTIME)lParam;

            if (((wParam & GDTR_MIN) && !IsValidDate(pst)) ||
                ((wParam & GDTR_MAX) && !IsValidDate(&pst[1])))
                break;

            // IE3 did not validate the time portion of this struct
            // use stToday time fields cuz pmc->stMin/Max may be zero
            if ((wParam & GDTR_MIN) && !IsValidTime(pst))
                CopyTime(pmc->stToday, pst[0]);
            if ((wParam & GDTR_MAX) && !IsValidTime(&pst[1]))
                CopyTime(pmc->stToday, pst[1]);

            if (wParam & GDTR_MIN)
            {
                pmc->stMin = *pst;
                pmc->fMinYrSet = TRUE;
            }
            else
            {
                pmc->stMin = c_stEpoch;
                pmc->fMinYrSet = FALSE;
            }
            pst++;
            if (wParam & GDTR_MAX)
            {
                pmc->stMax = *pst;
                pmc->fMaxYrSet = TRUE;
            }
            else
            {
                pmc->stMax = c_stArmageddon;
                pmc->fMaxYrSet = FALSE;
            }
            
            if (pmc->fMaxYrSet && pmc->fMinYrSet && CmpDate(&pmc->stMin, &pmc->stMax) > 0)
            {
                SYSTEMTIME stTemp = pmc->stMin;
                pmc->stMin = pmc->stMax;
                pmc->stMax = stTemp;
            }
            lres = TRUE;
        }
        break;

    case MCM_GETMONTHDELTA:
        if (pmc->fMonthDelta)
            lres = pmc->nMonthDelta;
        else
            lres = pmc->nMonths;
        break;

    case MCM_SETMONTHDELTA:
        if (pmc->fMonthDelta)
            lres = pmc->nMonthDelta;
        else
            lres = 0;
        if ((int)wParam==0)
            pmc->fMonthDelta = FALSE;
        else
        {
            pmc->fMonthDelta = TRUE;
            pmc->nMonthDelta = (int)wParam;
        }
        break;

    default:
        if (CCWndProc(&pmc->ci, uMsg, wParam, lParam, &lres))
            return lres;
        
        lres = DefWindowProc(hwnd, uMsg, wParam, lParam);
        break;
    } /* switch (uMsg) */

    return(lres);
}

LRESULT MCNcCreateHandler(HWND hwnd)
{
    MONTHCAL *pmc;

    // Allocate storage for the dtpick structure
    pmc = (MONTHCAL *)NearAlloc(sizeof(MONTHCAL));
    if (!pmc)
    {
        DebugMsg(DM_WARNING, TEXT("mc: Out Of Near Memory"));
        return(0L);
    }

    MonthCal_SetPtr(hwnd, pmc);

    return(1L);
}

void MCInitColorArray(COLORREF* pclr)
{
    pclr[MCSC_BACKGROUND]   = g_clrWindow;
    pclr[MCSC_MONTHBK]      = g_clrWindow;
    pclr[MCSC_TEXT]         = g_clrWindowText;
    pclr[MCSC_TITLEBK]      = GetSysColor(COLOR_ACTIVECAPTION);
    pclr[MCSC_TITLETEXT]    = GetSysColor(COLOR_CAPTIONTEXT);
    pclr[MCSC_TRAILINGTEXT] = g_clrGrayText;
}

LRESULT MCCreateHandler(MONTHCAL *pmc, HWND hwnd, LPCREATESTRUCT lpcs)
{
    HFONT      hfont;
    SYSTEMTIME st;

    // Validate data
    //
    if (lpcs->style & MCS_INVALIDBITS)
        return(-1);

    CIInitialize(&pmc->ci, hwnd, lpcs);
    UpdateLocaleInfo(pmc, &pmc->li);   

    // Initialize our data.
    //
    pmc->hinstance = lpcs->hInstance;
    
    pmc->fEnabled  = !(pmc->ci.style & WS_DISABLED);
    
    pmc->hpenToday = CreatePen(PS_SOLID, 2, CAL_COLOR_TODAY);

    MCReloadMenus(pmc);

    // Default minimum date is the epoch
    pmc->stMin = c_stEpoch;

    // Default maximum date is armageddon
    pmc->stMax = c_stArmageddon;

    GetLocalTime(&pmc->stToday);
    pmc->st = pmc->stToday;
    if (MonthCal_IsMultiSelect(pmc))
        pmc->stEndSel = pmc->st;

    // make sure the time portions of these are valid. they are never
    // touched after this point
    pmc->stMonthFirst = pmc->st;
    pmc->stMonthLast = pmc->st;
    pmc->stViewFirst = pmc->st;
    pmc->stViewLast = pmc->st;

    pmc->cSelMax = CAL_DEF_SELMAX;

    hfont = NULL;
    if (lpcs->hwndParent)
        hfont = (HFONT)SendMessage(lpcs->hwndParent, WM_GETFONT, 0, 0);
    if (hfont == NULL)
        hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    MCHandleSetFont(pmc, hfont, FALSE);
    
    CopyDate(pmc->st, st);
    // Can we start at January?
    if (st.wMonth <= (pmc->nViewRows * pmc->nViewCols))
        st.wMonth = 1;
    
    MCUpdateStartEndDates(pmc, &st);
    
    pmc->idTimerToday = SetTimer(pmc->ci.hwnd, CAL_TODAYTIMER, CAL_SECTODAYTIMER * 1000, NULL);

    MCInitColorArray(pmc->clr);

    return(0);
}

LRESULT MCOnStyleChanging(MONTHCAL *pmc, UINT gwl, LPSTYLESTRUCT pinfo)
{
    if (gwl == GWL_STYLE)
    {
        DWORD changeFlags = pmc->ci.style ^ pinfo->styleNew;

        // Don't allow these bits to change
        changeFlags &= MCS_MULTISELECT | MCS_DAYSTATE | MCS_INVALIDBITS;

        pinfo->styleNew ^= changeFlags;
    }

    return(0);
}

LRESULT MCOnStyleChanged(MONTHCAL *pmc, UINT gwl, LPSTYLESTRUCT pinfo)
{
    if (gwl == GWL_STYLE)
    {
        DWORD changeFlags = pmc->ci.style ^ pinfo->styleNew;

        ASSERT(!(changeFlags & (MCS_MULTISELECT|MCS_DAYSTATE|MCS_INVALIDBITS)));

        pmc->ci.style = pinfo->styleNew;

        if (changeFlags & MCS_WEEKNUMBERS)
        {
            MCCalcSizes(pmc);
            MCUpdateRcDayCur(pmc, &pmc->st);
            //MCUpdateToday(pmc);
        }

        // save a touch of code and share the MCUpdateToday
        // call with MCS_WEEKNUMBERS above
        if (changeFlags & MCS_NOTODAY|MCS_NOTODAYCIRCLE|MCS_WEEKNUMBERS)
        {
            MCUpdateToday(pmc);
        }

        if (changeFlags & (WS_BORDER | WS_CAPTION | WS_THICKFRAME)) {
            // the changing of these bits affect the size of the window
            // but not until after this message is handled
            // so post ourself a message.
            PostMessage(pmc->ci.hwnd, MCMP_WINDOWPOSCHANGED, 0, 0);
        }

        if (changeFlags)
            InvalidateRect(pmc->ci.hwnd, NULL, TRUE);
    }
    else if (gwl == GWL_EXSTYLE)
    {
        if ((pinfo->styleOld ^ pinfo->styleNew) & RTL_MIRRORED_WINDOW)
        {
            MCUpdateMonthNamePos(pmc);
        }
    }

    return(0);
}

void MCCalcSizes(MONTHCAL *pmc)
{
    HDC   hdc;
    HFONT hfontOrig;
    int   i, dxMax, dyMax, dxExtra;
    RECT  rect;
    TCHAR szBuf[128];
    TCHAR szDateFmt[64];

    // get sizing info for bold font...
    hdc = GetDC(pmc->ci.hwnd);
    hfontOrig = SelectObject(hdc, (HGDIOBJ)pmc->hfontBold);

    MGetTextExtent(hdc, g_szTextExtentDef, 2, &dxMax, &dyMax);
    MGetTextExtent(hdc, g_szTextExtentDef, 4, &pmc->dxYearMax, NULL);

    GetDateFormat(pmc->ct.lcid, DATE_SHORTDATE, &pmc->stToday,
        NULL, szDateFmt, sizeof(szDateFmt));
    wsprintf(szBuf,TEXT("%s %s"),pmc->li.szToday,szDateFmt);
    MGetTextExtent(hdc, szBuf, -1, &pmc->dxToday, &pmc->dyToday);
    // BUGBUG raymondc - hard-coded numbers are accessibility-incompatible
    pmc->dyToday += 4;

    //
    //  Cache these values so we don't go wacko if the app fails to
    //  forward WM_WININCHANGE messages into us and the user changes
    //  scrollbar widths.  We'll draw with the wrong width, but at
    //  least they will be consistently wrong.
    //
    pmc->dxArrowMargin = DX_ARROWMARGIN;
    pmc->dxCalArrow    = DX_CALARROW;
    pmc->dyCalArrow    = DY_CALARROW;

    //
    //  The banner bar consists of
    //
    //  margin + scrollbutton + spacer +
    //                      MonthName yyyy +
    //                        + spacer + scrollbutton + margin
    //
    //  Margin is dxArrowMargin
    //
    //  Scrollbutton = dxCalArrow
    //
    //  Spacer = border + CXVSCROLL + border
    //
    //  The spacer needs to be large enough for us to insert an updown
    //  control when it comes time to spin the year.  We don't need to
    //  cache the spacer anywhere - its value is implicit from the others.
    //
    //  The actual width is divided by the number of columns we need
    //  (typically 7, but perhaps 8 if we are also displaying week numbers).
    //
    //  We round the division down - later, we'll add some random futz
    //  to compensate.
    //
    dxExtra = pmc->dxArrowMargin + pmc->dxCalArrow +
                        (g_cxBorder + g_cxVScroll + g_cxBorder);
    dxExtra = dxExtra + dxExtra; // left + right

    for (i = 0; i < 12; i++)
    {
        int dxTemp;

        // BUGBUG raymondc - not localization safe for languages which change
        // month forms based on context
        wsprintf(szBuf,TEXT("%s %s"),pmc->li.rgszMonth[i],g_szTextExtentDef);
        
        MGetTextExtent(hdc, szBuf, -1, &dxTemp, NULL);
        dxTemp += dxExtra;
        dxTemp = dxTemp / (CALCOLMAX + (MonthCal_ShowWeekNumbers(pmc) ? 1:0));
        if (dxTemp > dxMax)
            dxMax = dxTemp;                    
    }
        
    SelectObject(hdc, (HGDIOBJ)pmc->hfont);
    for (i = 0; i < 7; i++)
    {
        SIZE  size;
        MGetTextExtent(hdc, pmc->li.rgszDay[i], -1, (LPINT)&size.cx, (LPINT)&size.cy);
        if (size.cx > dxMax)
            dxMax = size.cx;
        if (size.cy > dyMax)
            dyMax = size.cy;
    }

    if (dyMax < pmc->dyCalArrow / 2)
        dyMax = pmc->dyCalArrow / 2;

    SelectObject(hdc, (HGDIOBJ)hfontOrig);
    ReleaseDC(pmc->ci.hwnd, hdc);

    pmc->dxCol = dxMax + 2;
    pmc->dyRow = dyMax + 2;
    pmc->dxMonth = pmc->dxCol * (CALCOLMAX + (MonthCal_ShowWeekNumbers(pmc) ? 1:0)) + 1;
    pmc->dyMonth = pmc->dyRow * (CALROWMAX + 3) + 1; // we add 2 for the month name and day names

    pmc->dxToday += pmc->dxCol+6+CALBORDER; // +2 for -1 at ends and 4 for shift of circle 
    if (pmc->dxMonth > pmc->dxToday)
        pmc->dxToday = pmc->dxMonth;
     
    // Space for month name (tile bar area of each month)
    pmc->rcMonthName.left   = 0;
    pmc->rcMonthName.top    = 0;
    pmc->rcMonthName.right  = pmc->dxMonth;
    pmc->rcMonthName.bottom = pmc->rcMonthName.top + (pmc->dyRow * 2);

    // Space for day-of-week
    pmc->rcDow.left   = 0;
    pmc->rcDow.top    = pmc->rcMonthName.bottom;
    pmc->rcDow.right  = pmc->dxMonth;
    pmc->rcDow.bottom = pmc->rcDow.top + pmc->dyRow;

    // Space for week numbers
    if (MonthCal_ShowWeekNumbers(pmc))
    {
        pmc->rcWeekNum.left   = pmc->rcDow.left;
        pmc->rcWeekNum.top    = pmc->rcDow.bottom;
        pmc->rcWeekNum.right  = pmc->rcWeekNum.left + pmc->dxCol;
        pmc->rcWeekNum.bottom = pmc->dyMonth;

        pmc->rcDow.left  += pmc->dxCol;          // shift days of week
    }

    // Space for the day numbers
    pmc->rcDayNum.left   = pmc->rcDow.left;
    pmc->rcDayNum.top    = pmc->rcDow.bottom;
    pmc->rcDayNum.right  = pmc->rcDayNum.left + (CALCOLMAX * pmc->dxCol);
    pmc->rcDayNum.bottom = pmc->dyMonth;

    GetClientRect(pmc->ci.hwnd, &rect);

    MCRecomputeSizing(pmc, &rect);
}

void MCHandleSetFont(MONTHCAL *pmc, HFONT hfont, BOOL fRedraw)
{
    LOGFONT lf;
    HFONT   hfontBold;

    if (hfont == NULL)
        hfont = (HFONT)GetStockObject(SYSTEM_FONT);

    GetObject(hfont, sizeof(LOGFONT), (LPVOID)&lf);
    // we want to make sure that the bold days are obviously different
    // from the non-bold days...
    lf.lfWeight = (lf.lfWeight >= 700 ? 1000 : 800);
    hfontBold = CreateFontIndirect(&lf);

    if (hfontBold == NULL)
        return;

    if (pmc->hfontBold)
        DeleteObject((HGDIOBJ)pmc->hfontBold);

    pmc->hfont     = hfont;
    pmc->hfontBold = hfontBold;
    pmc->ci.uiCodePage = GetCodePageForFont(hfont);

    // calculate the new row and column sizes
    MCCalcSizes(pmc);

    if (fRedraw)
    {
        InvalidateRect(pmc->ci.hwnd, NULL, TRUE);
        UpdateWindow(pmc->ci.hwnd);
    }
}

#if 0       // why is this here?  it's not called anywhere??

// Stolen from the windows tips help file 
void DrawTransparentBitmap(HDC hdc, HBITMAP hbmp, RECT *prc, COLORREF cTransparentColor)
{
    COLORREF cColor;
    BITMAP bm;
    HBITMAP hbmAndBack, hbmAndObject, hbmAndMem, hbmSave;
    HGDIOBJ hbmBackOld, hbmObjectOld, hbmMemOld, hbmSaveOld;
    HDC hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave;
    POINT ptSize;
    int d;

    hdcTemp = CreateCompatibleDC(hdc);
    SelectObject(hdcTemp, (HGDIOBJ)hbmp);   // Select the bitmap

    GetObject(hbmp, sizeof(BITMAP), &bm);
    ptSize.x = bm.bmWidth;            // Get width of bitmap
    ptSize.y = bm.bmHeight;           // Get height of bitmap
    DPtoLP(hdcTemp, &ptSize, 1);      // Convert from device to logical points

    d = prc->right - prc->left;
    if (d < ptSize.x)
        ptSize.x = d;
    d = prc->bottom - prc->top;
    if (d < ptSize.y)
        ptSize.y = d;

    // Create some DCs to hold temporary data.
    hdcBack = CreateCompatibleDC(hdc);
    hdcObject = CreateCompatibleDC(hdc);
    hdcMem = CreateCompatibleDC(hdc);
    hdcSave = CreateCompatibleDC(hdc);

    // Create a bitmap for each DC. DCs are required for a number of
    // GDI functions.

    // Monochrome DC
    hbmAndBack = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

    // Monochrome DC
    hbmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

    hbmAndMem = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
    hbmSave = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);

    // Each DC must select a bitmap object to store pixel data.
    hbmBackOld = SelectObject(hdcBack, (HGDIOBJ)hbmAndBack);
    hbmObjectOld = SelectObject(hdcObject, (HGDIOBJ)hbmAndObject);
    hbmMemOld = SelectObject(hdcMem, (HGDIOBJ)hbmAndMem);
    hbmSaveOld = SelectObject(hdcSave, (HGDIOBJ)hbmSave);

    // Set proper mapping mode.
    SetMapMode(hdcTemp, GetMapMode(hdc));

    // Save the bitmap sent here, because it will be overwritten.
    BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

    // Set the background color of the source DC to the color.
    // contained in the parts of the bitmap that should be transparent
    cColor = SetBkColor(hdcTemp, cTransparentColor);

    // Create the object mask for the bitmap by performing a BitBlt
    // from the source bitmap to a monochrome bitmap.
    BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

    // Set the background color of the source DC back to the original
    // color.
    SetBkColor(hdcTemp, cColor);

    // Create the inverse of the object mask.
    BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, NOTSRCCOPY);

    // Copy the background of the main DC to the destination.
    BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, prc->left, prc->top, SRCCOPY);

    // Mask out the places where the bitmap will be placed.
    BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND);

    // Mask out the transparent colored pixels on the bitmap.
    BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);

    // XOR the bitmap with the background on the destination DC.
    BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);

    // Copy the destination to the screen.
    BitBlt(hdc, prc->left, prc->top, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY);

    // Place the original bitmap back into the bitmap sent here.
    BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY);

    // Delete the memory bitmaps.
    SelectObject(hdcBack, hbmBackOld);
    DeleteObject(hbmAndBack);
    SelectObject(hdcObject, hbmObjectOld);
    DeleteObject(hbmAndObject);
    SelectObject(hdcMem, hbmMemOld);
    DeleteObject(hbmAndMem);
    SelectObject(hdcSave, hbmSaveOld);
    DeleteObject(hbmSave);

    // Delete the memory DCs.
    DeleteDC(hdcMem);
    DeleteDC(hdcBack);
    DeleteDC(hdcObject);
    DeleteDC(hdcSave);
    DeleteDC(hdcTemp);
}

#endif      // DEAD CODE

void MCDrawTodayCircle(MONTHCAL *pmc, HDC hdc, RECT *prc)
{
    HGDIOBJ hpenOld;
    int xBegin, yBegin, yEnd;

    xBegin = (prc->right - prc->left) / 2 + prc->left;
    yBegin = prc->top + 4;
    yEnd = (prc->bottom - prc->top) / 2 + prc->top;

    hpenOld = SelectObject(hdc, (HGDIOBJ)pmc->hpenToday);
    Arc(hdc, prc->left + 1, yBegin, prc->right, prc->bottom,
        xBegin, yBegin, prc->right, yEnd); 
    Arc(hdc, prc->left - 10, prc->top + 1, prc->right, prc->bottom,
        prc->right, yEnd, prc->left + 3, yBegin);
    SelectObject(hdc, hpenOld);
}

void MCInvalidateMonthDays(MONTHCAL *pmc)
{
    InvalidateRect(pmc->ci.hwnd, &pmc->rcCentered, FALSE);
}

void MCGetTodayBtnRect(MONTHCAL *pmc, RECT *prc)
{    
    if (pmc->dxToday > pmc->rcCentered.right - pmc->rcCentered.left)
    {
        prc->left   = pmc->rc.left + 1;
        prc->right  = pmc->rc.right - 1;
    }
    else
    {
        prc->left   = pmc->rcCentered.left + 1;
        prc->right  = pmc->rcCentered.right - 1;
    }
    prc->top    = pmc->rcCentered.bottom - pmc->dyToday;
    prc->bottom = pmc->rcCentered.bottom;

    // center the today rect when we only have 1 col and it will fit in window
    if ((pmc->nViewCols == 1) && (pmc->dxToday <= pmc->rc.right - pmc->rc.left))
    {
        int dx =  ((pmc->rcCentered.right - pmc->rcCentered.left) - pmc->dxToday) / 2 - 1;
        prc->left   += dx;
        prc->right  -= dx;
    }
}

void MCPaintArrowBtn(MONTHCAL *pmc, HDC hdc, BOOL fPrev, BOOL fPressed)
{
    LPRECT prc;
    UINT   dfcs;
    BOOL   bMirrored = FALSE;

    // This is to work around DrawFrameControl() mirroring bug fixed on W2k (bld 2042 or higher)
#ifndef WINNT    
    bMirrored = IS_DC_RTL_MIRRORED(hdc);
#endif // WINNT    

    if (fPrev)
    {
        if(bMirrored)
        {
            dfcs = DFCS_SCROLLRIGHT;        
        }
        else
        {
            dfcs = DFCS_SCROLLLEFT;        
        }
        prc  = &pmc->rcPrev;
    }
    else
    {
        if(bMirrored)
        {
            dfcs = DFCS_SCROLLLEFT;        
        }
        else
        {
            dfcs = DFCS_SCROLLRIGHT;        
        }

        prc  = &pmc->rcNext;
    }
    if (pmc->fEnabled)
    {
        if (fPressed)
        {
            dfcs |= DFCS_PUSHED | DFCS_FLAT;
        }
    }
    else
    {
        dfcs |= DFCS_INACTIVE;
    }

    DrawFrameControl(hdc, prc, DFC_SCROLL, dfcs);
}

void MCPaint(MONTHCAL *pmc, HDC hdc)
{
    RECT    rc, rcT;
    int     irow, icol, iMonth, iYear, iIndex, dx, dy;
    HBRUSH  hbrSelect;
    HGDIOBJ hgdiOrig, hpenOrig;

    pmc->hpen = CreatePen(PS_SOLID, 0, pmc->clr[MCSC_TEXT]);
    hbrSelect = CreateSolidBrush(pmc->clr[MCSC_TITLEBK]);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, pmc->clr[MCSC_TEXT]);
    hpenOrig = SelectObject(hdc, GetStockObject(BLACK_PEN));

    rc = pmc->rcCentered;

    FillRectClr(hdc, &rc, pmc->clr[MCSC_MONTHBK]);

    SelectObject(hdc, (HGDIOBJ)pmc->hpen);

    // get the place for top left month
    rc.left   = pmc->rcCentered.left;
    rc.right  = rc.left + pmc->dxMonth;
    rc.top    = pmc->rcCentered.top;
    rc.bottom = rc.top + pmc->dyMonth;

    iMonth = pmc->stMonthFirst.wMonth;
    iYear  = pmc->stMonthFirst.wYear;

    dx = pmc->dxMonth + CALBORDER;
    dy = pmc->dyMonth + CALBORDER;

    iIndex = 0;
    for (irow = 0; irow < pmc->nViewRows; irow++)
    {
        rcT = rc;
        for (icol = 0; icol < pmc->nViewCols; icol++)
        {                
            if (RectVisible(hdc, &rcT))
            {
                MCPaintMonth(pmc, hdc, &rcT, iMonth, iYear, iIndex,
                    iIndex == 0, 
                    iIndex == (pmc->nMonths - 1), hbrSelect);
            }

            rcT.left  += dx;
            rcT.right += dx;

            if (++iMonth > 12)
            {
                iMonth = 1;
                iYear++;
            }

            iIndex++;
        }

        rc.top    += dy;
        rc.bottom += dy;
    }

    // draw the today stuff
    if (MonthCal_ShowToday(pmc))
    {
        MCGetTodayBtnRect(pmc, &rc);
        if (RectVisible(hdc, &rc))
        {
            TCHAR   szDateFmt[32];
            TCHAR   szBuf[64];

            rcT.right = rc.left + 2; // a bit extra border space
    
            if (MonthCal_ShowTodayCircle(pmc)) // this turns on/off the red circle
            {
                rcT.left   = rcT.right + 2;
                rcT.right  = rcT.left + pmc->dxCol - 2;
                rcT.top    = rc.top + 2;
                rcT.bottom = rc.bottom - 2;
                MCDrawTodayCircle(pmc, hdc, &rcT);
            }
    
            rcT.left   = rcT.right + 2;
            rcT.right  = rc.right - 2;
            rcT.top    = rc.top;
            rcT.bottom = rc.bottom;
            hgdiOrig = SelectObject(hdc, (HGDIOBJ)pmc->hfontBold);
            SetTextColor(hdc, pmc->clr[MCSC_TEXT]);
    
            GetDateFormat(pmc->ct.lcid, DATE_SHORTDATE, &pmc->stToday,
                            NULL, szDateFmt, sizeof(szDateFmt));
            wsprintf(szBuf, TEXT("%s %s"), pmc->li.szToday, szDateFmt);
            DrawText(hdc, szBuf, lstrlen(szBuf), &rcT,
                        DT_LEFT | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
            
            SelectObject(hdc, hgdiOrig);
        }
    }

    // Draw the spin buttons
    if (RectVisible(hdc, &pmc->rcPrev))
        MCPaintArrowBtn(pmc, hdc, TRUE, (pmc->idTimer && pmc->fSpinPrev));
    if (RectVisible(hdc, &pmc->rcNext))
        MCPaintArrowBtn(pmc, hdc, FALSE, (pmc->idTimer && !pmc->fSpinPrev));

    SelectObject(hdc, hpenOrig);

    DeleteObject((HGDIOBJ)hbrSelect);
    DeleteObject((HGDIOBJ)pmc->hpen);
}

//
//  MCGetMonthFormat gets the string to display for the month/year
//  in the passed-in SYSTEMTIME.  This is tricky because of eras.
//  If pmm is non-NULL, it receives the metrics of the
//  formatted month/year string.
//
void MCGetMonthFormat(MONTHCAL *pmc, SYSTEMTIME *pst, LPTSTR rgch, UINT cch, PMONTHMETRICS pmm)
{
    // For all months, we display the name appropriate to the first
    // day of the month.  Note that this means that the title of the
    // month in which the era changes may be confusing.  If the era
    // changes in the middle of a month, we name the month after the
    // previous era, even if the current selection belongs to the next
    // era.  I hope nobody will mind.

#if 0 // code that tried to track the era based on where the selection is
      // but before you can turn this on, you have to find everybody who
      // changes the selection, and that's hard because the monthcal control
      // doesn't have a centralizesd selection changer; people just party on
      // the selection directly
    if (pst->wMonth == pmc->st.wMonth &&
        pst->wYear == pmc->st.wYear) {
        pst->wDay = pmc->st.wDay;
    } else {
        pst->wDay = 1;
    }
#else
    pst->wDay = 1;
#endif

    //
    //  Get the string (all marked up), then extract the markers
    //  to locate the month and year substrings.
    //

    rgch[0] = TEXT('\0');       // In case something horrible happens
    GetDateFormat(pmc->ct.lcid, 0, pst,
                  pmc->li.szMonthYearFmt,
                  rgch, cch);
    MCRemoveMarkers(rgch, pmm);
}

void MCPaintMonth(MONTHCAL *pmc, HDC hdc, RECT *prc, int iMonth, int iYear, int iIndex,
                    BOOL fDrawPrev, BOOL fDrawNext, HBRUSH hbrSelect)
{
    BOOL fBold, fView, fReset;
    RECT rc, rcT;
    int nDay, cdy, irow, icol, crowShow, nweek, isel;
    TCHAR rgch[64];
    LPTSTR psz;
    HGDIOBJ hfontOrig, hbrushOld;
    COLORREF clrGrayText, clrHiliteText, clrOld, clrText;
    SYSTEMTIME st = {0};
    int iIndexSave = iIndex;

    clrText       = pmc->clr[MCSC_TEXT];
    clrGrayText   = pmc->clr[MCSC_TRAILINGTEXT];
    clrHiliteText = pmc->clr[MCSC_TITLETEXT];

    hfontOrig = SelectObject(hdc, (HGDIOBJ)pmc->hfont);
    SelectObject(hdc, (HGDIOBJ)pmc->hpen);

    //
    // Draw the Month and Year
    //
    // translate the relative coords to window coords 
    rc = pmc->rcMonthName;
    rc.left   += prc->left;
    rc.right  += prc->left;
    rc.top    += prc->top;
    rc.bottom += prc->top;
    if (RectVisible(hdc, &rc))
    {
        FillRectClr(hdc, &rc, pmc->clr[MCSC_TITLEBK]);

        SetTextColor(hdc, pmc->clr[MCSC_TITLETEXT]);
        SelectObject(hdc, (HGDIOBJ)pmc->hfontBold);

        st.wYear = (WORD) iYear;
        st.wMonth = (WORD) iMonth;
        MCGetMonthFormat(pmc, &st, rgch, ARRAYSIZE(rgch), NULL);

        DrawText(hdc, rgch, lstrlen(rgch), &rc, DT_CENTER | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

#ifdef MARKER_DEBUG
        //
        //  When debugging MCInsertMarker and MCRemoveMarker, draw colored
        //  bars where we think the markers were.
        //
        { RECT rcT = rc;
            rcT.top = rcT.bottom - 2;
            rcT.left  = rc.left + pmc->rgmm[iIndex].rgi[IMM_MONTHSTART];
            rcT.right = rc.left + pmc->rgmm[iIndex].rgi[IMM_MONTHEND];
            FillRectClr(hdc, &rcT, RGB(0xFF, 0, 0));

            rcT.left  = rc.left + pmc->rgmm[iIndex].rgi[IMM_YEARSTART];
            rcT.right = rc.left + pmc->rgmm[iIndex].rgi[IMM_YEAREND];
            FillRectClr(hdc, &rcT, RGB(0, 0xFF, 0));
        }
#endif
        SelectObject(hdc, (HGDIOBJ)pmc->hfont);
    }

    SetTextColor(hdc, pmc->clr[MCSC_TITLEBK]);

    //
    // Draw the days of the month
    //
    // translate the relative coords to window coords 
    rc = pmc->rcDow;
    rc.left   += prc->left;
    rc.right  += prc->left;
    rc.top    += prc->top;
    rc.bottom += prc->top;
    if (RectVisible(hdc, &rc))
    {
        MoveToEx(hdc, rc.left + 4, rc.bottom - 1, NULL);
        LineTo(hdc, rc.right - 4, rc.bottom - 1);

        rc.right = rc.left + pmc->dxCol;

        for (icol = 0; icol < CALCOLMAX; icol++)
        {
            psz = pmc->li.rgszDay[(icol + pmc->li.dowStartWeek) % 7];

            DrawText(hdc, psz, lstrlen(psz), &rc, DT_CENTER | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
            rc.left  += pmc->dxCol;
            rc.right += pmc->dxCol;
        }
    }

    
    // Check to see how many days from the previous month exist in this months calendar
    nDay = pmc->rgnDayUL[iIndex];   // last day in prev month that won't be shown in this month
    cdy  = pmc->rgcDay[iIndex];     // # of days in prev month

    // Calculate the number of weeks to display
    if (fDrawNext)
        crowShow = CALROWMAX;
    else
        crowShow = ((cdy - nDay) + pmc->rgcDay[iIndex + 1] + 6/* round up */) / 7;

    if (nDay != cdy)
    {
        // start at previous month
        iMonth--;
        if(iMonth <= 0)
        {
            iMonth = 12;
            iYear--;
        }
        nDay++;

        fView = FALSE;
    }
    else
    {
        // start at this month
        iIndex++;                   // this month

        nDay = 1;
        cdy = pmc->rgcDay[iIndex];

        fView = TRUE;
    }

    //
    // Draw the week numbers
    //
    if (MonthCal_ShowWeekNumbers(pmc))
    {
        // translate the relative coords to window coords 
        rc = pmc->rcWeekNum;
        rc.left   += prc->left;
        rc.top    += prc->top;
        rc.right  += prc->left;
        rc.bottom = rc.top + (pmc->dyRow * crowShow);

        // draw the week numbers
        if (RectVisible(hdc, &rc))
        {
            MoveToEx(hdc, rc.right - 1, rc.top + 4, NULL);
            LineTo(hdc, rc.right - 1, rc.bottom - 4);

            st.wYear  = (WORD) iYear;
            st.wMonth = (WORD) iMonth;
            st.wDay   = (WORD) nDay;
            nweek = GetWeekNumber(&st, pmc->li.dowStartWeek, pmc->li.firstWeek);

            rc.bottom = rc.top + pmc->dyRow;

            for (irow = 0; irow < crowShow; irow++)
            {
                wsprintf(rgch, g_szNumFmt, nweek);
                DrawText(hdc, rgch, (nweek > 9 ? 2 : 1), &rc,
                        DT_CENTER | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
                
                rc.top    += pmc->dyRow;
                rc.bottom += pmc->dyRow;
                IncrSystemTime(&st, &st, 1, INCRSYS_WEEK);
                nweek = GetWeekNumber(&st, pmc->li.dowStartWeek, pmc->li.firstWeek);
            }
        }
    }

    if (!fView)
        SetTextColor(hdc, clrGrayText);
    else
        SetTextColor(hdc, clrText);

    rc = pmc->rcDayNum;
    rc.left   += prc->left;
    rc.top    += prc->top;
    rc.right  =  rc.left + pmc->dxCol;
    rc.bottom =  rc.top  + pmc->dyRow;

    fReset = FALSE;
    fBold  = FALSE;

    for (irow = 0; irow < crowShow; irow++)
    {
        rcT = rc;

        for (icol = 0; icol < CALCOLMAX; icol++)
        {
            if ((fView || fDrawPrev) && RectVisible(hdc, &rcT))
            {
                wsprintf(rgch, g_szNumFmt, nDay);

                if (MonthCal_IsDayState(pmc))
                {
                    // if we're in a dropdown we don't display 
                    if (MCIsBoldOffsetDay(pmc, nDay, iIndex))
                    {
                        if (!fBold)
                        {
                            SelectObject(hdc, (HGDIOBJ)pmc->hfontBold);
                            fBold = TRUE;
                        }
                    }
                    else
                    {
                        if (fBold)
                        {
                            SelectObject(hdc, (HGDIOBJ)pmc->hfont);
                            fBold = FALSE;
                        }
                    }
                }

                if (isel = MCIsSelectedDayMoYr(pmc, nDay, iMonth, iYear))
                {
                    int x1, x2;

                    clrOld    = SetTextColor(hdc, clrHiliteText);
                    hbrushOld = SelectObject(hdc, (HGDIOBJ)hbrSelect);
                    fReset    = TRUE;

                    SelectObject(hdc, GetStockObject(NULL_PEN));
                     
                    x1 = 0;
                    x2 = 0;
                    if (isel & SEL_DOT)
                    {
                        Ellipse(hdc, rcT.left + 2, rcT.top + 2, rcT.right - 1, rcT.bottom - 1);
                        if (isel == SEL_BEGIN)
                        {
                            x1 = rcT.left + (rcT.right - rcT.left) / 2;
                            x2 = rcT.right;
                        }
                        else if (isel == SEL_END)
                        {
                            x1 = rcT.left;
                            x2 = rcT.left + (rcT.right - rcT.left) / 2;
                        }
                    }
                    else
                    {
                        x1 = rcT.left;
                        x2 = rcT.right;
                    }

                    if (x1 && x2)
                    {
                        Rectangle(hdc, x1, rcT.top + 2, x2 + 1, rcT.bottom - 1);
                    }
                }

                DrawText(hdc, rgch, (nDay > 9 ? 2 : 1), &rcT,
                        DT_CENTER | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

                if (MonthCal_ShowTodayCircle(pmc) && pmc->fToday && iIndexSave == pmc->iMonthToday &&
                    icol == pmc->iColToday && irow == pmc->iRowToday)
                {
                    MCDrawTodayCircle(pmc, hdc, &rcT);
                }

                if (fReset)
                {
                    SetTextColor(hdc, clrOld);
                    SelectObject(hdc, (HGDIOBJ)hbrushOld);
                    fReset = FALSE;
                }
            }

            rcT.left  += pmc->dxCol;
            rcT.right += pmc->dxCol;

            nDay++;
            if (nDay > cdy)
            {
                if (!fDrawNext && iIndex > iIndexSave)
                    goto doneMonth;

                nDay = 1;
                iIndex++;
                cdy = pmc->rgcDay[iIndex];
                iMonth++;
                if (iMonth > 12)
                {
                    iMonth = 1;
                    iYear++;
                }

                fView = !fView;
                SetTextColor(hdc, fView ? clrText : clrGrayText);

                fDrawPrev = fDrawNext;
            }
        }

        rc.top    += pmc->dyRow;
        rc.bottom += pmc->dyRow;
    }

doneMonth:

    SelectObject(hdc, hfontOrig);

    return;
}

int MCIsSelectedDayMoYr(MONTHCAL *pmc, int iDay, int iMonth, int iYear)
{
    SYSTEMTIME st;
    int iBegin, iEnd;
    int iret = 0;

    st.wYear  = (WORD) iYear;
    st.wMonth = (WORD) iMonth;
    st.wDay   = (WORD) iDay;

    iBegin = CmpDate(&st, &pmc->st);

    if (MonthCal_IsMultiSelect(pmc))
    {
        iEnd = CmpDate(&st, &pmc->stEndSel);

        if (iBegin > 0 && iEnd< 0)
            iret = SEL_MID;
        else
        {
            if (iBegin == 0)
                iret |= SEL_BEGIN;
        
            if (iEnd == 0)
                iret |= SEL_END;
        }
    }
    else if (iBegin == 0)
    {
        iret = SEL_DOT;
    }

    return(iret);
}

BOOL MCIsBoldOffsetDay(MONTHCAL *pmc, int nDay, int iIndex)
{
    return(pmc->rgdayState && (pmc->rgdayState[iIndex] & (1L << (nDay - 1))) != 0);
}

void MCNcDestroyHandler(HWND hwnd, MONTHCAL *pmc, WPARAM wParam, LPARAM lParam)
{
    if (pmc)
    {
        if (pmc->hpenToday)
            DeleteObject((HGDIOBJ)pmc->hpenToday);
        if (pmc->hfontBold)
            DeleteObject((HGDIOBJ)pmc->hfontBold);
        
        if (pmc->hmenuCtxt)
            DestroyMenu(pmc->hmenuCtxt);
        if (pmc->hmenuMonth)
            DestroyMenu(pmc->hmenuMonth);

        if (pmc->idTimer)
            KillTimer(pmc->ci.hwnd, pmc->idTimer);
        if (pmc->idTimerToday)
            KillTimer(pmc->ci.hwnd, pmc->idTimerToday);

        MCFreeCalendarInfo(&pmc->ct);

        GlobalFreePtr(pmc);
    }

    // In case rogue messages float through after we have freed the pdtpick, set
    // the handle in the window structure to FFFF and test for this value at 
    // the top of the WndProc 
    MonthCal_SetPtr(hwnd, NULL);

    // Call DefWindowProc32 to free all little chunks of memory such as szName 
    // and rgwScroll.  
    DefWindowProc(hwnd, WM_NCDESTROY, wParam, lParam);
}


/* Computes the following:
 *  nViewCols
 *  nViewRows
 *  rcCentered
 *  rcPrev
 *  rcNext
 */
void MCRecomputeSizing(MONTHCAL *pmc, RECT *prect)
{
    RECT rc;
    int dx, dy, dCal;

    // Space for entire calendar
    pmc->rc = *prect;

    dx = prect->right  - prect->left;
    dy = prect->bottom - prect->top;

    pmc->nViewCols = 1 + (dx - pmc->dxMonth) / (pmc->dxMonth + CALBORDER);
    pmc->nViewRows = 1 + (dy - pmc->dyMonth - pmc->dyToday) / (pmc->dyMonth + CALBORDER);

    // if dx < dxMonth or dy < dyMonth, these can be zero. That's bad...
    if (pmc->nViewCols < 1)
        pmc->nViewCols = 1;
    if (pmc->nViewRows < 1)
        pmc->nViewRows = 1;

    // Make sure we don't display more than CALMONTHMAX months
    while ((pmc->nViewRows * pmc->nViewCols) > CALMONTHMAX)
    {
        if (pmc->nViewRows > pmc->nViewCols)
            pmc->nViewRows--;
        else
            pmc->nViewCols--;
    }

    // RC for the months, centered within the client window
    dCal = pmc->nViewCols * (pmc->dxMonth + CALBORDER) - CALBORDER;
    pmc->rcCentered.left = (dx - dCal) / 2;
    if (pmc->rcCentered.left < 0)
        pmc->rcCentered.left = 0;
    pmc->rcCentered.right = pmc->rcCentered.left + dCal;

    dCal = pmc->nViewRows * (pmc->dyMonth + CALBORDER) - CALBORDER + pmc->dyToday;
    pmc->rcCentered.top = (dy - dCal) / 2;
    if (pmc->rcCentered.top < 0)
        pmc->rcCentered.top = 0;
    pmc->rcCentered.bottom = pmc->rcCentered.top + dCal;

    // Calculate and set RCs for the spin buttons
    rc.top    = pmc->rcCentered.top + (pmc->dyRow * 2 - pmc->dyCalArrow) /2;
    rc.bottom = rc.top + pmc->dyCalArrow;

    rc.left  = pmc->rcCentered.left + pmc->dxArrowMargin;
    rc.right = rc.left + pmc->dxCalArrow;
    pmc->rcPrev = rc;

    rc.right = pmc->rcCentered.right - pmc->dxArrowMargin;
    rc.left  = rc.right - pmc->dxCalArrow;
    pmc->rcNext = rc;
}

LRESULT MCSizeHandler(MONTHCAL *pmc, RECT *prc)
{
    int nMax;
    SYSTEMTIME st;
    int cmo, dmo;

    MCRecomputeSizing(pmc, prc);

    nMax = pmc->nViewRows * pmc->nViewCols;

    
    // Compute new start date
    CopyDate(pmc->stMonthFirst, st);

    // BUGBUG: this doesn't consider stEndSel
    cmo = (pmc->stMonthLast.wYear - (int)pmc->st.wYear) * 12 +
        (pmc->stMonthLast.wMonth - (int)pmc->st.wMonth);
    dmo = nMax - pmc->nMonths;

    if (-dmo > cmo)
    {
        // Selected mon/yr not in view
        IncrSystemTime(&st, &st, -(cmo + dmo), INCRSYS_MONTH);
        cmo = 0;
    }

    // If the # of months being displayed has changed, then lets try to
    // start the calendar from January.
    if ((dmo != 0) && (cmo + dmo >= pmc->stMonthFirst.wMonth - 1))
        st.wMonth = 1;

    MCUpdateStartEndDates(pmc, &st);

    InvalidateRect(pmc->ci.hwnd, NULL, TRUE);
    UpdateWindow(pmc->ci.hwnd);

    return(0);
}

//
//  For each month being displayed, compute the precise locations of all
//  the gizmos we draw into the month header area.
//
void MCUpdateMonthNamePos(MONTHCAL *pmc)
{
    HDC hdc;
    int iCount;
    SYSTEMTIME st;
    TCHAR rgch[64];
    SIZE size;
    HGDIOBJ hfontOrig;

    hdc = GetDC(pmc->ci.hwnd);
    hfontOrig = SelectObject(hdc, (HGDIOBJ)pmc->hfontBold);

    st = pmc->stMonthFirst;

    for (iCount = 0; iCount < pmc->nMonths; iCount++)
    {
        PMONTHMETRICS pmm = &pmc->rgmm[iCount];
        int i;

        MCGetMonthFormat(pmc, &st, rgch, ARRAYSIZE(rgch), pmm);

        GetTextExtentPoint32(hdc, rgch, lstrlen(rgch), &size);
        pmm->rgi[IMM_START] = (pmc->dxMonth - size.cx) / 2;

        //
        //  Now convert the indices into pixels so we can figure out where
        //  all the strings ended up.
        //
        for (i = IMM_DATEFIRST; i <= IMM_DATELAST; i++) {
            SIZE sizeT;
            // In case of horrible error, pretend the marker was at the
            // beginning of the string.
            sizeT.cx = 0;
            GetTextExtentPoint32(hdc, rgch, pmm->rgi[i], &sizeT);
            pmm->rgi[i] = pmm->rgi[IMM_START] + sizeT.cx;
        }

        //
        //  Now flip the coordinates for RTL.
        //
        if (pmc->fHeaderRTL || IS_WINDOW_RTL_MIRRORED(pmc->ci.hwnd))
        {
            int dxStart, dxEnd;

            // Flip the month...
            dxStart = pmm->rgi[IMM_MONTHSTART] - pmm->rgi[IMM_START];
            dxEnd   = pmm->rgi[IMM_MONTHEND  ] - pmm->rgi[IMM_START];
            pmm->rgi[IMM_MONTHSTART] = pmm->rgi[IMM_START] + size.cx - dxEnd;
            pmm->rgi[IMM_MONTHEND  ] = pmm->rgi[IMM_START] + size.cx - dxStart;

            // Flip the year...
            dxStart = pmm->rgi[IMM_YEARSTART] - pmm->rgi[IMM_START];
            dxEnd   = pmm->rgi[IMM_YEAREND  ] - pmm->rgi[IMM_START];
            pmm->rgi[IMM_YEARSTART] = pmm->rgi[IMM_START] + size.cx - dxEnd;
            pmm->rgi[IMM_YEAREND  ] = pmm->rgi[IMM_START] + size.cx - dxStart;

        }

        //  On to the next month

        if(++st.wMonth > 12)
        {
            st.wMonth = 1;
            st.wYear++;
        }
    }

    SelectObject(hdc, hfontOrig);
    ReleaseDC(pmc->ci.hwnd, hdc);
}

/*
 * Computes the following, given the number of rows & columns available:
 *        stMonthFirst.wMonth
 *        stMonthFirst.wYear
 *        stMonthLast.wMonth
 *        stMonthLast.wYear
 *        nMonths
 *
 * Trashes *pstStart
 */
void MCUpdateStartEndDates(MONTHCAL *pmc, SYSTEMTIME *pstStart)
{
    int iCount, iMonth, iYear;
    int nMonthsToEdge;

    pmc->nMonths = pmc->nViewRows * pmc->nViewCols;

    // make sure pstStart to pstStart+nMonths is within range
    nMonthsToEdge = ((int)pmc->stMax.wYear - (int)pstStart->wYear) * 12 +
                        ((int)pmc->stMax.wMonth - (int)pstStart->wMonth) + 1;
    if (nMonthsToEdge < pmc->nMonths)
        IncrSystemTime(pstStart, pstStart, nMonthsToEdge - pmc->nMonths, INCRSYS_MONTH);

    if (CmpDate(pstStart, &pmc->stMin) < 0)
    {
        CopyDate(pmc->stMin, *pstStart);
    }

    nMonthsToEdge = ((int)pmc->stMax.wYear - (int)pstStart->wYear) * 12 +
                        ((int)pmc->stMax.wMonth - (int)pstStart->wMonth) + 1;
    if (nMonthsToEdge < pmc->nMonths)
        pmc->nMonths = nMonthsToEdge;

    pmc->stMonthFirst.wYear  = pstStart->wYear;
    pmc->stMonthFirst.wMonth = pstStart->wMonth;
    pmc->stMonthFirst.wDay   = 1;
    if (CmpDate(&pmc->stMonthFirst, &pmc->stMin) < 0)
    {
        pmc->stMonthFirst.wDay = pmc->stMin.wDay;
        ASSERT(0==CmpDate(&pmc->stMonthFirst, &pmc->stMin));
    }

    // these ranges are CALMONTHMAX+2 and nMonths <= CALMONTHMAX, so we are safe
    // index 0 corresponds to stViewFirst (DAYSTATE) info
    // index 1..nMonths correspond to stMonthFirst..stMonthLast info
    // index nMonths+1 corresponds to stViewLast (DAYSTATE) info
    //
    iYear  = pmc->stMonthFirst.wYear;
    iMonth = pmc->stMonthFirst.wMonth - 1;
    if(iMonth == 0)
    {
        iMonth = 12;
        iYear--;
    }
    for (iCount = 0; iCount <= pmc->nMonths+1; iCount++)
    {
        int cdy, dow, ddow;

        // number of days in this month
        cdy = GetDaysForMonth(iYear, iMonth);
        pmc->rgcDay[iCount] = cdy;

        // move to "this" month
        if(++iMonth > 12)
        {
            iMonth = 1;
            iYear++;
        }

        // last day of this month NOT visible when viewing NEXT month
        dow = GetStartDowForMonth(iYear, iMonth);
        ddow = dow - pmc->li.dowStartWeek;
        if(ddow < 0)
            ddow += CALCOLMAX;
        pmc->rgnDayUL[iCount] = cdy  - ddow;
    }

    // we want to always have days visible on the previous month
    if (pmc->rgnDayUL[0] == pmc->rgcDay[0])
        pmc->rgnDayUL[0] -= CALCOLMAX;

    IncrSystemTime(&pmc->stMonthFirst, &pmc->stMonthLast, pmc->nMonths - 1, INCRSYS_MONTH);
    pmc->stMonthLast.wDay = (WORD) pmc->rgcDay[pmc->nMonths];
    if (pmc->fMaxYrSet && CmpDate(&pmc->stMonthLast, &pmc->stMax) > 0)
    {
        pmc->stMonthLast.wDay = pmc->stMax.wDay;
        ASSERT(0==CmpDate(&pmc->stMonthLast, &pmc->stMax));
    }
    
    pmc->stViewFirst.wYear  = pmc->stMonthFirst.wYear;
    pmc->stViewFirst.wMonth = pmc->stMonthFirst.wMonth - 1;
    if (pmc->stViewFirst.wMonth == 0)
    {
        pmc->stViewFirst.wMonth = 12;
        pmc->stViewFirst.wYear--;
    }
    pmc->stViewFirst.wDay = pmc->rgnDayUL[0] + 1;

    pmc->stViewLast.wYear  = pmc->stMonthLast.wYear;
    pmc->stViewLast.wMonth = pmc->stMonthLast.wMonth + 1;
    if (pmc->stViewLast.wMonth == 13)
    {
        pmc->stViewLast.wMonth = 1;
        pmc->stViewLast.wYear++;
    }
    // total days - (days in last month + remaining days in previous month)
    pmc->stViewLast.wDay = CALROWMAX * CALCOLMAX -
        (pmc->rgcDay[pmc->nMonths] +
         pmc->rgcDay[pmc->nMonths-1] - pmc->rgnDayUL[pmc->nMonths-1]);

    MCUpdateDayState(pmc);
    MCUpdateRcDayCur(pmc, &pmc->st);
    MCUpdateToday(pmc);
    MCUpdateMonthNamePos(pmc);
}

void MCUpdateToday(MONTHCAL *pmc)
{
    if (MonthCal_ShowTodayCircle(pmc))
    {
        int iMonth;

        iMonth = MCGetOffsetForYrMo(pmc, pmc->stToday.wYear, pmc->stToday.wMonth);
        if (iMonth < 0)
        {
            // today is not visible in the displayed months
            pmc->fToday = FALSE;
        }
        else
        {
            int iDay;

            // today is visible in the displayed months
            pmc->fToday = TRUE;
            
            iDay = pmc->rgcDay[iMonth] - pmc->rgnDayUL[iMonth] + pmc->stToday.wDay - 1;
    
            pmc->iMonthToday = iMonth;
            pmc->iRowToday   = iDay / CALCOLMAX;
            pmc->iColToday   = iDay % CALCOLMAX;
        }
    }
}

BOOL FUpdateRcDayCur(MONTHCAL *pmc, POINT pt)
{
    int iRow, iCol;
    RECT rc;
    SYSTEMTIME st;
    
    if (!FGetDateForPt(pmc, pt, &st, NULL, &iCol, &iRow, &rc))
        return FALSE;

    if (CmpDate(&st, &pmc->stMin) < 0)
        return FALSE;

    if (CmpDate(&st, &pmc->stMax) > 0)
        return FALSE;

    // calculate the day rc
    pmc->rcDayCur.left   = rc.left + pmc->rcDayNum.left + iCol * pmc->dxCol;
    pmc->rcDayCur.top    = rc.top + pmc->rcDayNum.top + iRow * pmc->dyRow;
    pmc->rcDayCur.right  = pmc->rcDayCur.left + pmc->dxCol;
    pmc->rcDayCur.bottom = pmc->rcDayCur.top + pmc->dyRow;

    return(TRUE);
}

void MCUpdateDayState(MONTHCAL *pmc)
{
    HWND hwndParent;

    if (!MonthCal_IsDayState(pmc))
        return;

    hwndParent = GetParent(pmc->ci.hwnd);
    if (hwndParent)
    {
        int i, mon, yr, cmonths;

        yr      = pmc->stViewFirst.wYear;
        mon     = pmc->stViewFirst.wMonth;
        cmonths = pmc->nMonths + 2;

        // don't do anything unless we need to
        if (cmonths != pmc->cds || mon != pmc->dsMonth || yr != pmc->dsYear)
        {
            // this is a small enough to not deal with allocating it
            NMDAYSTATE    nmds;
            MONTHDAYSTATE buffer[CALMONTHMAX+2];

            ZeroMemory(&nmds, sizeof(nmds));
            nmds.stStart.wYear  = (WORD) yr;
            nmds.stStart.wMonth = (WORD) mon;
            nmds.stStart.wDay   = 1;
            nmds.cDayState      = cmonths;
            nmds.prgDayState    = buffer;

            CCSendNotify(&pmc->ci, MCN_GETDAYSTATE, &nmds.nmhdr);

            for (i = 0; i < cmonths; i++)
                pmc->rgdayState[i] = nmds.prgDayState[i];

            pmc->cds     = cmonths;
            pmc->dsMonth = mon;
            pmc->dsYear  = yr;
        }
    }
}

void MCNotifySelChange(MONTHCAL *pmc, UINT uMsg)
{
    HWND hwndParent;

    if (pmc->fNoNotify)
        return;

    hwndParent = GetParent(pmc->ci.hwnd);
    if (hwndParent)
    {
        NMSELCHANGE nmsc;
        ZeroMemory(&nmsc, sizeof(nmsc));

        CopyDate(pmc->st, nmsc.stSelStart);
        if (MonthCal_IsMultiSelect(pmc))
            CopyDate(pmc->stEndSel, nmsc.stSelEnd);

        CCSendNotify(&pmc->ci, uMsg, &nmsc.nmhdr);
    }
}

void MCUpdateRcDayCur(MONTHCAL *pmc, SYSTEMTIME *pst)
{
    int iOff;

    iOff = MCGetOffsetForYrMo(pmc, pst->wYear, pst->wMonth);
    if (iOff >= 0)
        MCGetRcForDay(pmc, iOff, pst->wDay, &pmc->rcDayCur);
}

// returns zero-based index into DISPLAYED months for month
// if month is not in DISPLAYED months, then -1 is returned...
int MCGetOffsetForYrMo(MONTHCAL *pmc, int iYear, int iMonth)
{
    int iOff;

    iOff = ((int)iYear - pmc->stMonthFirst.wYear) * 12 + (int)iMonth - pmc->stMonthFirst.wMonth;

    if (iOff < 0 || iOff >= pmc->nMonths)
        return(-1);

    return(iOff);
}

// iMonth is a zero-based index relative to the DISPLAYED months.
// iDay is a 1-based index of the day of the month,
void MCGetRcForDay(MONTHCAL *pmc, int iMonth, int iDay, RECT *prc)
{
    RECT rc;
    int iPlace, iRow, iCol;

    MCGetRcForMonth(pmc, iMonth, &rc);

    iPlace = pmc->rgcDay[iMonth] - pmc->rgnDayUL[iMonth] + iDay - 1;
    iRow = iPlace / CALCOLMAX;
    iCol = iPlace % CALCOLMAX;

    prc->left   = rc.left   + pmc->rcDayNum.left + (pmc->dxCol * iCol);
    prc->top    = rc.top    + pmc->rcDayNum.top  + (pmc->dyRow * iRow);
    prc->right  = prc->left + pmc->dxCol;
    prc->bottom = prc->top  + pmc->dyRow;
}

//
// This routine gets the bounding rect for the iMonth of the displayed months.
// NOTE: iMonth is a zero-based index relative to the DISPLAYED months,
// counting along the rows.
//
void MCGetRcForMonth(MONTHCAL *pmc, int iMonth, RECT *prc)
{
    int iRow, iCol, d;

    iRow = iMonth / pmc->nViewCols;
    iCol = iMonth % pmc->nViewCols;

    // intialize the rect to be the bounding rect for the month in the
    // top left corner
    prc->left   = pmc->rcCentered.left;
    prc->right  = prc->left + pmc->dxMonth;
    prc->top    = pmc->rcCentered.top;
    prc->bottom = prc->top + pmc->dyMonth;

    if (iCol)       // slide the rect across to the correct column
    {
        d = (pmc->dxMonth + CALBORDER) * iCol;
        prc->left  += d;
        prc->right += d;
    }
    if (iRow)       // slide the rect down to the correct row
    {
        d = (pmc->dyMonth + CALBORDER) * iRow;
        prc->top    += d;
        prc->bottom += d;
    }
}

// Changes starting month by nDelta
// returns number of months actually changed
int FIncrStartMonth(MONTHCAL *pmc, int nDelta, BOOL fNoCurDayChange)
{
    SYSTEMTIME stStart;

    int nOldStartYear  = pmc->stMonthFirst.wYear;
    int nOldStartMonth = pmc->stMonthFirst.wMonth;

    IncrSystemTime(&pmc->stMonthFirst, &stStart, nDelta, INCRSYS_MONTH);

    // MCUpdateStartEndDates takes stMin/stMax into account
    MCUpdateStartEndDates(pmc, &stStart);

    if (!fNoCurDayChange)
    {
        int cday;

        // BUGBUG: we arbitrarily set the currently selected day
        // to be in the new stMonthFirst, but given the way the
        // control works, I doubt we ever hit this code. what's it for??

        if (MonthCal_IsMultiSelect(pmc))
            cday = DaysBetweenDates(&pmc->st, &pmc->stEndSel);

        // need to set date for focus here
        pmc->st.wMonth = pmc->stMonthFirst.wMonth;
        pmc->st.wYear  = pmc->stMonthFirst.wYear;

        // Check to see if the day is in range, eg, Jan 31 -> Feb 28
        if (pmc->st.wDay > pmc->rgcDay[1])
            pmc->st.wDay = (WORD) pmc->rgcDay[1];

        if (MonthCal_IsMultiSelect(pmc))
            IncrSystemTime(&pmc->st, &pmc->stEndSel, cday, INCRSYS_DAY);

        MCNotifySelChange(pmc, MCN_SELCHANGE);

        MCUpdateRcDayCur(pmc, &pmc->st);
    }

    MCInvalidateMonthDays(pmc);

    return((pmc->stMonthFirst.wYear-nOldStartYear)*12 + (pmc->stMonthFirst.wMonth-nOldStartMonth));
}

// FIncrStartMonth with a beep when it doesn't change.
int MCIncrStartMonth(MONTHCAL *pmc, int nDelta, BOOL fDelayDayChange)
{
    int cmoSpun;

    // FIncrStartMonth takes stMin/stMax into account
    cmoSpun = FIncrStartMonth(pmc, nDelta, fDelayDayChange);

    if (cmoSpun==0)
        MessageBeep(0);

    return(cmoSpun);
}

//
// Determines in which month the given point lies.  In other words, if the
// calendar control is currently sized to show six months, this routine
// determines in which which of those six months the point lies.  It returns
// the zero based index of the month, counting along the rows.
//
BOOL FGetOffsetForPt(MONTHCAL *pmc, POINT pt, int *piOffset)
{
    int iRow, iCol, i;

    // check to see if point is within the centered months
    if (!PtInRect(&pmc->rcCentered, pt))
        return(FALSE);

    // calculate the month row and column
    // (we're really fudging a little here, since the point could
    // actually be within the space between months...)
    iCol = (pt.x - pmc->rcCentered.left) / (pmc->dxMonth + CALBORDER);
    iRow = (pt.y - pmc->rcCentered.top) / (pmc->dyMonth + CALBORDER);

    i = iRow * pmc->nViewCols + iCol;
    if (i >= pmc->nMonths)
        return(FALSE);

    *piOffset = i;

    return(TRUE);
}

//
// This routine returns the row and column of day containing the given point
//
BOOL FGetRowColForRelPt(MONTHCAL *pmc, POINT ptRel, int *piRow, int *piCol)
{
    if (!PtInRect(&pmc->rcDayNum, ptRel))
        return(FALSE);

    ptRel.x -= pmc->rcDayNum.left;
    ptRel.y -= pmc->rcDayNum.top;

    *piCol = ptRel.x / pmc->dxCol;
    *piRow = ptRel.y / pmc->dyRow;

    return(TRUE);
}

//
// This routine returns the month and year of the iMonth in the displayed
// months.  NOTE: iMonth is a zero-based index of the displayed months
//
void GetYrMoForOffset(MONTHCAL *pmc, int iMonth, int *piYear, int *piMonth)
{
    SYSTEMTIME st;

    st.wDay   = 1;
    st.wMonth = pmc->stMonthFirst.wMonth;
    st.wYear  = pmc->stMonthFirst.wYear;
    
    IncrSystemTime(&st, &st, iMonth, INCRSYS_MONTH);

    *piYear  = st.wYear;
    *piMonth = st.wMonth;
}

//
// This routine returns, the day, month, and year of day containing the
// given point.  It will optionally return the day of the month, the row and
// column in the month, and the bounding rect of the month containing the point.
// NOTE: the day returned in piDay can be less than 1 (to indicate a day in the
// previous month) or greater than the number of days in the month (to indicate
// a day in the next month).
//
BOOL FGetDateForPt(MONTHCAL *pmc, POINT pt, SYSTEMTIME *pst, int *piDay,
                   int* piCol, int* piRow, LPRECT prcMonth)
{
    int iOff, iRow, iCol, iDay, iMon, iYear;
    RECT rcMonth;

    if (!FGetOffsetForPt(pmc, pt, &iOff))
        return(FALSE);

    MCGetRcForMonth(pmc, iOff, &rcMonth);
    pt.x -= rcMonth.left;
    pt.y -= rcMonth.top;
    if (!FGetRowColForRelPt(pmc, pt, &iRow, &iCol))
        return(FALSE);

    // get the day containing the point by subtracting the number of days
    // that are visible from the previous month, and then add one, since
    // we are zero-based and the days of the month are 1-based.
    //
    iDay = iRow * CALCOLMAX + iCol - (pmc->rgcDay[iOff] - pmc->rgnDayUL[iOff]) + 1;
    if (piDay)
        *piDay = iDay;
    
    if (iDay <= 0)
    {
        if (iOff)
            return(FALSE);      // dont accept days in prev month unless
                                // this happens to be the first month
                                
        iDay += pmc->rgcDay[iOff];  // add the cnt of days in the prev month,
        --iOff;                     // then incr the month to get day in new month
    }
    else if (iDay > pmc->rgcDay[iOff+1])
    {
        if (iOff < (pmc->nMonths - 1))  // dont accept days in next month unless
            return(FALSE);              // this happens to be the last month
        
        ++iOff;                         // increment the month, and then sub the
        iDay -= pmc->rgcDay[iOff];      // count of days to get day in new month
    }

    GetYrMoForOffset(pmc, iOff, &iYear, &iMon);
    pst->wDay   = (WORD) iDay;
    pst->wMonth = (WORD) iMon;
    pst->wYear  = (WORD) iYear;

    if (piCol)
        *piCol = iCol;
    
    if (piRow)
        *piRow = iRow;
    
    if (prcMonth)
        *prcMonth = rcMonth;
    
    return(TRUE);
}

BOOL MCSetDate(MONTHCAL *pmc, SYSTEMTIME *pst)
{
    int nDelta = 0;
    
    //
    // Can't set date outside of min/max range
    //
    if (CmpDate(pst, &pmc->stMin) < 0)
        return FALSE;
    if (CmpDate(pst, &pmc->stMax) > 0)
        return FALSE;

    //
    // Set new day
    //
    pmc->st = *pst;
    if (MonthCal_IsMultiSelect(pmc))
        pmc->stEndSel = *pst;

    FScrollIntoView(pmc);
    
    MCNotifySelChange(pmc, MCN_SELCHANGE);

    MCUpdateRcDayCur(pmc, pst);

    return(TRUE);
}

void MCSetToday(MONTHCAL* pmc, SYSTEMTIME* pst)
{
    SYSTEMTIME st;
    RECT rc;
    
    if (!pst)
    {
        GetLocalTime(&st);
        pmc->fTodaySet = FALSE;
    }
    else
    {
        st = *pst;
        pmc->fTodaySet = TRUE;
    }
    
    if (CmpDate(&st, &pmc->stToday) != 0)
    {
        MCGetRcForDay(pmc, pmc->iMonthToday, pmc->stToday.wDay, &rc);
        InvalidateRect(pmc->ci.hwnd, &rc, FALSE);

        pmc->stToday = st;

        MCUpdateToday(pmc);

        MCGetRcForDay(pmc, pmc->iMonthToday, pmc->stToday.wDay, &rc);
        InvalidateRect(pmc->ci.hwnd, &rc, FALSE);

        if (MonthCal_ShowToday(pmc))
        {
            MCGetTodayBtnRect(pmc, &rc);
            InvalidateRect(pmc->ci.hwnd, &rc, FALSE);
        }

        UpdateWindow(pmc->ci.hwnd);
    }
}

LRESULT MCHandleTimer(MONTHCAL *pmc, WPARAM wParam)
{
    if (wParam == CAL_IDAUTOSPIN)
    {
        int nDelta = pmc->fMonthDelta ? pmc->nMonthDelta : pmc->nMonths;

        // BUGBUG pass last parameter TRUE if multiselect! else you
        // can't multiselect across months
        MCIncrStartMonth(pmc, (pmc->fSpinPrev ? -nDelta : nDelta), FALSE);

        if (pmc->idTimer == 0)
            pmc->idTimer = SetTimer(pmc->ci.hwnd, CAL_IDAUTOSPIN, CAL_MSECAUTOSPIN, NULL);

        pmc->rcDayOld = pmc->rcDayCur;
        UpdateWindow(pmc->ci.hwnd);
    }
    else if (wParam == CAL_TODAYTIMER)
    {
        if (!pmc->fTodaySet)
            MCSetToday(pmc, NULL);
    }
    
    MCNotifySelChange(pmc, MCN_SELCHANGE);     // our date has changed
    
    return((LRESULT)TRUE);
}

void MCInvalidateDates(MONTHCAL *pmc, SYSTEMTIME *pst1, SYSTEMTIME *pst2)
{
    int iMonth, ioff, icol, irow;
    RECT rc, rcMonth;
    SYSTEMTIME st, stEnd;

    if (CmpDate(pst1, &pmc->stViewLast) > 0 ||
        CmpDate(pst2, &pmc->stViewFirst) < 0)
        return;
        
    if (CmpDate(pst1, &pmc->stViewFirst) < 0)
        CopyDate(pmc->stViewFirst, st);
    else
        CopyDate(*pst1, st);

    if (CmpDate(pst2, &pmc->stViewLast) > 0)
        CopyDate(pmc->stViewLast, stEnd);
    else
        CopyDate(*pst2, stEnd);

    iMonth = MCGetOffsetForYrMo(pmc, st.wYear, st.wMonth);
    if (iMonth == -1)
    {
        if (st.wMonth == pmc->stViewFirst.wMonth)
        {
            iMonth = 0;
            ioff = st.wDay - pmc->rgnDayUL[0] - 1;
        }
        else
        {
            iMonth = pmc->nMonths - 1;
            ioff = st.wDay + pmc->rgcDay[pmc->nMonths] +
                pmc->rgcDay[iMonth] - pmc->rgnDayUL[iMonth] - 1;
        }
    }
    else
    {
        ioff = st.wDay + (pmc->rgcDay[iMonth] - pmc->rgnDayUL[iMonth]) - 1;
    }

    MCGetRcForMonth(pmc, iMonth, &rcMonth);

    // TODO: this is bullshit. make it more efficient...
    while (CmpDate(&st, &stEnd) <= 0)
    {
        irow = ioff / CALCOLMAX;
        icol = ioff % CALCOLMAX;
        rc.left   = rcMonth.left + pmc->rcDayNum.left + (pmc->dxCol * icol);
        rc.top    = rcMonth.top  + pmc->rcDayNum.top  + (pmc->dyRow * irow);
        rc.right  = rc.left      + pmc->dxCol;
        rc.bottom = rc.top       + pmc->dyRow;

        InvalidateRect(pmc->ci.hwnd, &rc, FALSE);

        IncrSystemTime(&st, &st, 1, INCRSYS_DAY);
        ioff++;

        if (st.wDay == 1)
        {
            if (st.wMonth != pmc->stMonthFirst.wMonth &&
                st.wMonth != pmc->stViewLast.wMonth)
            {
                iMonth++;
                MCGetRcForMonth(pmc, iMonth, &rcMonth);

                ioff = ioff % CALCOLMAX;
            }
        }
    }
}

void MCHandleMultiSelect(MONTHCAL *pmc, SYSTEMTIME *pst)
{
    int i;
    DWORD cday;
    SYSTEMTIME stStart, stEnd;

    if (!pmc->fMultiSelecting)
    {
        CopyDate(*pst, stStart);
        CopyDate(*pst, stEnd);

        pmc->fMultiSelecting = TRUE;
        pmc->fForwardSelect = TRUE;

        CopyDate(pmc->st, pmc->stStartPrev);
        CopyDate(pmc->stEndSel, pmc->stEndPrev);
    }
    else
    {
        if (pmc->fForwardSelect)
        {
            i = CmpDate(pst, &pmc->st);
            if (i >= 0)
            {
                CopyDate(pmc->st, stStart);
                CopyDate(*pst, stEnd);
            }
            else
            {
                CopyDate(*pst, stStart);
                CopyDate(pmc->st, stEnd);
                pmc->fForwardSelect = FALSE;
            }
        }
        else
        {
            i = CmpDate(pst, &pmc->stEndSel);
            if (i < 0)
            {
                CopyDate(*pst, stStart);
                CopyDate(pmc->stEndSel, stEnd);
            }
            else
            {
                CopyDate(pmc->stEndSel, stStart);
                CopyDate(*pst, stEnd);
                pmc->fForwardSelect = TRUE;
            }
        }
    }
    
    // check to make sure not exceeding cSelMax
    cday = DaysBetweenDates(&stStart, &stEnd) + 1;
    if (cday > pmc->cSelMax)
    {
        if (pmc->fForwardSelect)
            IncrSystemTime(&stStart, &stEnd, pmc->cSelMax - 1, INCRSYS_DAY);
        else
            IncrSystemTime(&stEnd, &stStart, 1 - pmc->cSelMax, INCRSYS_DAY);
    }

    if (0 == CmpDate(&stStart, &pmc->st) &&
        0 == CmpDate(&stEnd, &pmc->stEndSel))
        return;

    // TODO: do this more effeciently..
    MCInvalidateDates(pmc, &pmc->st, &pmc->stEndSel);
    MCInvalidateDates(pmc, &stStart, &stEnd);

    CopyDate(stStart, pmc->st);
    CopyDate(stEnd, pmc->stEndSel);

    MCNotifySelChange(pmc, MCN_SELCHANGE);

    UpdateWindow(pmc->ci.hwnd);
}

void MCGotoToday(MONTHCAL *pmc)
{
    pmc->rcDayOld = pmc->rcDayCur;

    // force old selection to get repainted
    if (MonthCal_IsMultiSelect(pmc))
        MCInvalidateDates(pmc, &pmc->st, &pmc->stEndSel);
    else
        InvalidateRect(pmc->ci.hwnd, &pmc->rcDayOld, FALSE);

    MCSetDate(pmc, &pmc->stToday);
    
    MCNotifySelChange(pmc, MCN_SELECT);    
    
    // force new selection to get repainted
    InvalidateRect(pmc->ci.hwnd, &pmc->rcDayCur, FALSE);
    UpdateWindow(pmc->ci.hwnd);
}

LRESULT MCContextMenu(MONTHCAL *pmc, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    int click;

    if (!pmc->fEnabled || !MonthCal_ShowToday(pmc))
        return(0);

    // ignore double click since this makes us advance twice
    // since we already had a leftdown before the leftdblclk
    if (!pmc->fCapture)
    {
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);

        //
        //  If the context menu was generated from the keyboard,
        //  then put it at the focus rectangle.
        //
        if (pt.x == -1 && pt.y == -1)
        {
            pt.x = (pmc->rcDayCur.left + pmc->rcDayCur.right ) / 2;
            pt.y = (pmc->rcDayCur.top  + pmc->rcDayCur.bottom) / 2;
            ClientToScreen(pmc->ci.hwnd, &pt);
        }

        click = TrackPopupMenu(pmc->hmenuCtxt,
                    TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
                    pt.x, pt.y, 0, pmc->ci.hwnd, NULL);
        if (click >= 1)
            MCGotoToday(pmc);
    }

    return(0);
}

//
// Computes the bounding rects for the month and the year in the title area of
// the month.
//
void MCGetTitleRcsForOffset(MONTHCAL* pmc, int iOffset, LPRECT prcMonth, LPRECT prcYear)
{
    RECT rcT;
    RECT rc;
    MCGetRcForMonth(pmc, iOffset, &rc);
    
    rcT.top    = rc.top + (pmc->dyRow / 2);
    rcT.bottom = rcT.top + pmc->dyRow;

    rcT.left  = rc.left + pmc->rcMonthName.left + pmc->rgmm[iOffset].rgi[IMM_MONTHSTART];
    rcT.right = rc.left + pmc->rcMonthName.left + pmc->rgmm[iOffset].rgi[IMM_MONTHEND];
    *prcMonth = rcT;

    rcT.left  = rc.left + pmc->rcMonthName.left + pmc->rgmm[iOffset].rgi[IMM_YEARSTART];
    rcT.right = rc.left + pmc->rcMonthName.left + pmc->rgmm[iOffset].rgi[IMM_YEAREND];
    *prcYear  = rcT;

}

LRESULT MCLButtonDown(MONTHCAL *pmc, WPARAM wParam, LPARAM lParam)
{
    HDC        hdc;
    POINT      pt;
    SYSTEMTIME st;
    RECT       rc, rcCal;
    BOOL       fShow;
    MSG        msg;
    int        offset, imonth, iyear;

    if (!pmc->fEnabled)
        return(0);

    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);
  
    // treat a shift click like an LMouseDown at the prev location and
    // a MouseMove to the new location
    if (MonthCal_IsMultiSelect(pmc) && ((wParam & MK_SHIFT) == MK_SHIFT) && (!PtInRect(&pmc->rcDayCur, pt)))
    {
        SetCapture(pmc->ci.hwnd);
        pmc->fCapture = TRUE;
        
        pmc->fForwardSelect = (CmpDate(&pmc->stAnchor, &pmc->st) != 0) ? FALSE : TRUE;
        pmc->fMultiSelecting = TRUE;

        hdc = GetDC(pmc->ci.hwnd);
        DrawFocusRect(hdc, &pmc->rcDayCur);    // draw focus rect
        pmc->fFocusDrawn = TRUE;
        ReleaseDC(pmc->ci.hwnd, hdc);
        
        MCMouseMove(pmc, wParam, lParam);      // draw the highlight to new date

        return 0;
    }
    
    // ignore double click since this makes us advance twice
    // since we already had a leftdown before the leftdblclk
    if (!pmc->fCapture)
    {
        SetCapture(pmc->ci.hwnd);
        pmc->fCapture = TRUE;

        // check for spin buttons
        if ((pmc->fSpinPrev = (WORD) PtInRect(&pmc->rcPrev, pt)) || PtInRect(&pmc->rcNext, pt))
        {
            MCHandleTimer(pmc, CAL_IDAUTOSPIN);

            return(0);
        }
                      
        // check for valid day
        pmc->rcDayOld = pmc->rcDayCur;   // rcDayCur should always be valid now

        if (MonthCal_IsMultiSelect(pmc))
        {
            // need to cache these values because these are how
            // we determine if the selection has changed and we
            // need to notify the parent
            CopyDate(pmc->st, pmc->stStartPrev);
            CopyDate(pmc->stEndSel, pmc->stEndPrev);
        }

                
        if (FUpdateRcDayCur(pmc, pt))
        {
            if (MonthCal_IsMultiSelect(pmc))
            {
                if (FGetDateForPt(pmc, pt, &st, NULL, NULL, NULL, NULL))
                    MCHandleMultiSelect(pmc, &st);
            }

            hdc = GetDC(pmc->ci.hwnd);
            DrawFocusRect(hdc, &pmc->rcDayCur);    // draw focus rect
            pmc->fFocusDrawn = TRUE;
            ReleaseDC(pmc->ci.hwnd, hdc);

            CopyDate(st, pmc->stAnchor);           // new Anchor point
        }
        else
        {
            RECT rcMonth, rcYear;
            int delta, year, month;
            
            // is this a click in the today area...
            if (MonthCal_ShowToday(pmc))
            {
                MCGetTodayBtnRect(pmc, &rc);
                if (PtInRect(&rc, pt))
                {
                    CCReleaseCapture(&pmc->ci);
                    pmc->fCapture = FALSE;
    
                    MCGotoToday(pmc);
                    return(0);
                }
            }

            // figure out if the click was in a month name or a year

            if (!FGetOffsetForPt(pmc, pt, &offset))
                return(0);

            GetYrMoForOffset(pmc, offset, &year, &month);

            // calculate where the month name and year are,
            // so we can figure out if they clicked in them...
            MCGetTitleRcsForOffset(pmc, offset, &rcMonth, &rcYear);
            
            delta = 0;
            if (PtInRect(&rcMonth, pt))
            {
                CCReleaseCapture(&pmc->ci);
                pmc->fCapture = FALSE;

                ClientToScreen(pmc->ci.hwnd, &pt);
                imonth = TrackPopupMenu(pmc->hmenuMonth,
                    TPM_LEFTALIGN | TPM_TOPALIGN |
                    TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                    pt.x, pt.y, 0, pmc->ci.hwnd, NULL);
                if (imonth >= 1)
                    delta = imonth - month;
                goto ChangeMonth;
            }                

            if (PtInRect(&rcYear, pt))
            {
                HWND hwndEdit, hwndUD, hwndFocus;
                int yrMin, yrMax;
                DWORD dwExStyle = 0L;
                CCReleaseCapture(&pmc->ci);
                pmc->fCapture = FALSE;

                
                //
                // If the year is in a RTL string, then numeric control
                // is to the left. 
                //
                if (pmc->fHeaderRTL)
                {
                    rcYear.left = (rcYear.right - (pmc->dxYearMax + 6));
                }
                else
                {
                    rcYear.right = rcYear.left + pmc->dxYearMax + 6;
                }
                rcYear.top--;
                rcYear.bottom++;
                if(((pmc->fHeaderRTL) && !(IS_WINDOW_RTL_MIRRORED(pmc->ci.hwnd))) ||
                  (!(pmc->fHeaderRTL) && (IS_WINDOW_RTL_MIRRORED(pmc->ci.hwnd))))
                {
                    // not mirrored force RTL, mirrored force LTR (for mirroring RTLis LTR!!)
                    dwExStyle|= WS_EX_RTLREADING;
                }
                hwndEdit = CreateWindowEx(dwExStyle, TEXT("EDIT"), NULL,
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY | ES_LEFT | ES_AUTOHSCROLL,
                    rcYear.left, rcYear.top, rcYear.right - rcYear.left, rcYear.bottom - rcYear.top,
                    pmc->ci.hwnd, (HMENU)0, pmc->hinstance, NULL);
                if (hwndEdit == NULL)
                    return(0);

                pmc->hwndEdit = hwndEdit;

                SendMessage(hwndEdit, WM_SETFONT, (WPARAM)pmc->hfontBold, (LPARAM)FALSE);
                SendMessage(hwndEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                            (LPARAM)MAKELONG(1, 1));
                MCUpdateEditYear(pmc);

                //
                //  Convert from Gregorian to display years.
                //
                year = GregorianToOther(&pmc->ct, year);
                yrMin = GregorianToOther(&pmc->ct, pmc->stMin.wYear);
                yrMax = 9999;
                if (pmc->fMaxYrSet)
                    yrMax = GregorianToOther(&pmc->ct, pmc->stMax.wYear);

                hwndUD = CreateUpDownControl(
                    WS_CHILD | WS_VISIBLE | WS_BORDER | 
                    UDS_NOTHOUSANDS | UDS_ARROWKEYS,// | UDS_SETBUDDYINT,
                    pmc->fHeaderRTL ? (rcYear.left - 1 - (rcYear.bottom-rcYear.top)): (rcYear.right + 1), 
                    rcYear.top, 
                    rcYear.bottom - rcYear.top, rcYear.bottom - rcYear.top, pmc->ci.hwnd,
                    1, pmc->hinstance, hwndEdit, yrMax, yrMin, year);
                if (hwndUD == NULL)
                {
                    DestroyWindow(hwndEdit);
                    return(0);
                }

                pmc->hwndUD = hwndUD;

                hwndFocus = SetFocus(hwndEdit);

                //
                // Widen the area depending on the string direction.
                //
                if (pmc->fHeaderRTL)
                    rcYear.left -= (1 + rcYear.bottom - rcYear.top);
                else
                    rcYear.right += 1 + rcYear.bottom - rcYear.top;
                // Use MapWindowRect, It works in a mirrored and unmirrored windows.
                MapWindowRect(pmc->ci.hwnd, NULL, (LPPOINT)&rcYear);

                rcCal = pmc->rc;
                MapWindowRect(pmc->ci.hwnd, NULL, (LPPOINT)&rcCal);

                fShow = TRUE;

                while (fShow && GetFocus() == hwndEdit)
                {
                    if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
                    {
                        // Check for events that cause the calendar to go away

                        if (msg.message == WM_KILLFOCUS ||
                            (msg.message >= WM_SYSKEYDOWN &&
                            msg.message <= WM_SYSDEADCHAR))
                        {
                            fShow = FALSE;
                        }
                        else if ((msg.message == WM_LBUTTONDOWN ||
                            msg.message == WM_NCLBUTTONDOWN ||
                            msg.message == WM_RBUTTONDOWN ||
                            msg.message == WM_NCRBUTTONDOWN ||
                            msg.message == WM_MBUTTONDOWN ||
                            msg.message == WM_NCMBUTTONDOWN) &&
                            !PtInRect(&rcYear, msg.pt))
                        {
                            fShow = FALSE;
                            
                            // if its a button down inside the calendar, eat it
                            // so the calendar doesn't do anything strange when
                            // the user is just trying to get rid of the year edit
                            if (PtInRect(&rcCal, msg.pt))
                                GetMessage(&msg, NULL, 0, 0);
                            
                            break;    // do not dispatch
                        }
                        else if (msg.message == WM_QUIT)
                        {   // Don't dispatch a WM_QUIT; leave it in the queue
                            break;    // do not dispatch
                        }
                        else if (msg.message == WM_CHAR)
                        {
                            if (msg.wParam == VK_ESCAPE)
                            {
                                goto NoYearChange;
                            }
                            else if (msg.wParam == VK_RETURN)
                            {
                                fShow = FALSE;
                            }
                        }

                        GetMessage(&msg, NULL, 0, 0);
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                    else
                        WaitMessage();
                }

                iyear = (int) SendMessage(hwndUD, UDM_GETPOS, 0, 0);
                if (HIWORD(iyear) == 0)
                    delta = (iyear - year) * 12;

NoYearChange:
                DestroyWindow(hwndUD);
                DestroyWindow(hwndEdit);

                pmc->hwndUD = NULL;
                pmc->hwndEdit = NULL;

                UpdateWindow(pmc->ci.hwnd);

                if (hwndFocus != NULL)
                    SetFocus(hwndFocus);
            }
ChangeMonth:
            if (delta != 0)
            {
                MCIncrStartMonth(pmc, delta, FALSE);
                MCNotifySelChange(pmc,MCN_SELCHANGE);
            }
            
        }
    }

    return(0);
}

LRESULT MCLButtonUp(MONTHCAL *pmc, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    SYSTEMTIME st;
    POINT pt;
        
    if (pmc->fCapture)
    {
        CCReleaseCapture(&pmc->ci);
        pmc->fCapture = FALSE;

        if (pmc->idTimer)
        {
            KillTimer(pmc->ci.hwnd, pmc->idTimer);
            pmc->idTimer = 0;

            hdc = GetDC(pmc->ci.hwnd);
            MCPaintArrowBtn(pmc, hdc, pmc->fSpinPrev, FALSE);
            ReleaseDC(pmc->ci.hwnd, hdc);

            return(0);
        }


        if (pmc->fFocusDrawn)
        {
            hdc = GetDC(pmc->ci.hwnd);
            DrawFocusRect(hdc, &pmc->rcDayCur); // erase old focus rect
            pmc->fFocusDrawn = FALSE;
            ReleaseDC(pmc->ci.hwnd, hdc);
        }

        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);

        if (MonthCal_IsMultiSelect(pmc))
        {
            FUpdateRcDayCur(pmc, pt);

            if (!EqualRect(&pmc->rcDayOld, &pmc->rcDayCur))
            {
                if (FGetDateForPt(pmc, pt, &st, NULL, NULL, NULL, NULL))
                    MCHandleMultiSelect(pmc, &st);
            }

            pmc->fMultiSelecting = FALSE;
            if (0 != CmpDate(&pmc->stStartPrev, &pmc->st) ||
                0 != CmpDate(&pmc->stEndPrev, &pmc->stEndSel))
            {
                FScrollIntoView(pmc);
            }
            MCNotifySelChange(pmc, MCN_SELECT);
        }
        else
        {
            if (FUpdateRcDayCur(pmc, pt))
            {
                if (!EqualRect(&pmc->rcDayOld, &pmc->rcDayCur) && (FGetDateForPt(pmc, pt, &st, NULL, NULL, NULL, NULL)))
                {
                    InvalidateRect(pmc->ci.hwnd, &pmc->rcDayOld, FALSE);
                    InvalidateRect(pmc->ci.hwnd, &pmc->rcDayCur, FALSE);

                    MCSetDate(pmc, &st);
                }
                
                MCNotifySelChange(pmc, MCN_SELECT);
            }
        }
    }

    return(0);
}

LRESULT MCMouseMove(MONTHCAL *pmc, WPARAM wParam, LPARAM lParam)
{
    BOOL fPrev;
    HDC hdc;
    POINT pt;
    SYSTEMTIME st;

    if (pmc->fCapture)
    {
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);

        // check spin buttons
        if ((fPrev = PtInRect(&pmc->rcPrev, pt)) || PtInRect(&pmc->rcNext, pt))
        {
            if (pmc->idTimer == 0)
            {
                pmc->fSpinPrev = (WORD) fPrev;
                MCHandleTimer(pmc, CAL_IDAUTOSPIN);
            }

            return(0);
        }
        else
        {
            hdc = GetDC(pmc->ci.hwnd);

            if (pmc->idTimer)
            {
                KillTimer(pmc->ci.hwnd, pmc->idTimer);
                pmc->idTimer = 0;
                MCPaintArrowBtn(pmc, hdc, pmc->fSpinPrev, FALSE);
            }
        }

        // check days
        if (!PtInRect(&pmc->rcDayCur, pt))
        {
            if (pmc->fFocusDrawn)
                DrawFocusRect(hdc, &pmc->rcDayCur);         // erase focus rect

            if (pmc->fFocusDrawn = (WORD) FUpdateRcDayCur(pmc, pt))
            {
                // moved into a new valid day
                if (pmc->fMultiSelecting)
                {
                    if (FGetDateForPt(pmc, pt, &st, NULL, NULL, NULL, NULL))
                        MCHandleMultiSelect(pmc, &st);
                }

                DrawFocusRect(hdc, &pmc->rcDayCur);
            }
            else
            {
                // moved into an invalid position
                pmc->rcDayCur = pmc->rcDayOld;
            }
        }
        else if (!pmc->fFocusDrawn)
        {
            // handle case where we just moved back into rcDayCur from invalid area
            DrawFocusRect(hdc, &pmc->rcDayCur);
            pmc->fFocusDrawn = TRUE;
        }

        ReleaseDC(pmc->ci.hwnd, hdc);
    }

    return(0);
}


LRESULT MCHandleKeydown(MONTHCAL *pmc, WPARAM wParam, LPARAM lParam)
{
    LONG       lIncrement;
    int        iDirection;
    SYSTEMTIME st;
    BOOL       fRet = FALSE;
    HDC        hdc = NULL;
    RECT       rcCurFocus;

    // BUGBUG raymondc ERA - need to invalidate month title when selection
    // moves in/out/within

    switch (wParam)
    {
        case VK_CONTROL:
            pmc->fControl = TRUE;           // we'll clear this on WM_KEYUP
            return TRUE;
            break;

        case VK_SHIFT:
            pmc->fShift = TRUE;             // we'll clear this on WM_KEYUP
            return TRUE;
            break;
            
        case VK_LEFT:                       // goto previous day
            iDirection = -1;
            lIncrement = INCRSYS_DAY;
            break;
            
        case VK_RIGHT:                      // goto next day
            iDirection = 1;
            lIncrement = INCRSYS_DAY;
            break;

        case VK_UP:                         // goto previous week
            iDirection = -1;
            lIncrement = INCRSYS_WEEK;
            break;

        case VK_DOWN:                       // goto next week
            iDirection = 1;
            lIncrement = INCRSYS_WEEK;
            break;

        case VK_NEXT:               
            iDirection = 1;
            if (pmc->fControl)              // goto next year
                lIncrement = INCRSYS_YEAR;
            else                            // goto next month
                lIncrement = INCRSYS_MONTH;
            break;

        case VK_PRIOR:
            iDirection = -1;
            if (pmc->fControl)              // goto previous year
                lIncrement = INCRSYS_YEAR;  
            else
                lIncrement = INCRSYS_MONTH; // goto next month
            break;

        case VK_HOME:
            if (pmc->fControl)              // goto first visible month
            {
                CopyDate(pmc->stMonthFirst, st);
            }
            else                            // goto first day of current month
            {
                CopyDate(pmc->st, st);
                st.wDay = 1;
            }
            goto setDate;                  
            break;

        case VK_END:
            if (pmc->fControl)              // goto last visible month
            {
                CopyDate(pmc->stMonthLast, st);
            }
            else                            // goto last day of current month
            {                       
                CopyDate(pmc->st, st);
                st.wDay = (WORD) GetDaysForMonth(st.wYear, st.wMonth);
            }
            goto setDate;
            break;
            
            
        default:
            return FALSE;
    }

    // if we're multiselecting, we need to know which "end" of the selection
    // the user is moving.

    if (pmc->fMultiSelecting && pmc->fForwardSelect)
        CopyDate(pmc->stEndSel, st);
    else
        CopyDate(pmc->st, st);

    IncrSystemTime(&st, &st, iDirection, lIncrement);


setDate:

    // based on the window style and the shift key state,
    // we'll do a multi-select (or not)
    if (MonthCal_IsMultiSelect(pmc) && pmc->fShift)
    {
        pmc->fForwardSelect = (CmpDate(&pmc->st, &pmc->stAnchor) >= 0) ? TRUE : FALSE;
        pmc->fMultiSelecting = TRUE;
    }

    // otherwise, we'll end multiselect, and set the new anchor
    else
    {
        pmc->fMultiSelecting = FALSE;
        CopyDate(st, pmc->stAnchor);
    }
        
    if (pmc->fFocusDrawn)   // erase the focus rect, but don't clear the bit
    {                       // so we know to put it back
        hdc = GetDC(pmc->ci.hwnd);
        DrawFocusRect(hdc, &pmc->rcDayCur);
        ReleaseDC(pmc->ci.hwnd, hdc);
        rcCurFocus = pmc->rcDayCur;
    }
    else
    {
        pmc->rcDayOld = pmc->rcDayCur;
    }

    if (MonthCal_IsMultiSelect(pmc))
    {
        int nDelta = 0;

        MCHandleMultiSelect(pmc, &st);                

        FScrollIntoView(pmc);
    }                                                
    else if (fRet = MCSetDate(pmc, &st))
    {
        InvalidateRect(pmc->ci.hwnd, &pmc->rcDayOld, FALSE);
        InvalidateRect(pmc->ci.hwnd, &pmc->rcDayCur, FALSE);
        UpdateWindow(pmc->ci.hwnd);
    }
    
    if (pmc->fFocusDrawn)   // put the focus rect back
    {
        pmc->rcDayOld = pmc->rcDayCur;
        pmc->rcDayCur = rcCurFocus;
        hdc = GetDC(pmc->ci.hwnd);
        DrawFocusRect(hdc, &pmc->rcDayCur);
        ReleaseDC(pmc->ci.hwnd, hdc);
    }

    return fRet;
}

#if 0 // coming soon

LRESULT MCHandleChar(MONTHCAL *pmc, WPARAM wParam, LPARAM lParam)
{


    return 0;
}

#endif

//
//  Era information is kept in a DPA of LocalAlloc'd strings.
//
int MCDPAEnumCallback(LPVOID d, LPVOID p)
{
    UNREFERENCED_PARAMETER(p);
    if (d)
        LocalFree(d);
    return TRUE;
}

void MCDPADestroy(HDPA hdpa)
{
    if (hdpa)
        DPA_DestroyCallback(hdpa, MCDPAEnumCallback, 0);
}

//
//  Collect era information.
//
//  Since EnumCalendarInfo is not thread-safe, we have to take the critical
//  section.

HDPA g_hdpaCal;

#ifdef WINNT
BOOL MCEnumCalInfoProc(LPWSTR psz)
#else
BOOL MCEnumCalInfoProc(LPSTR psz)
#endif
{
#ifdef UNICODE_WIN9x
    LPWSTR pwszSave = StrDup_AtoW((LPCWSTR)psz);
#else
    LPWSTR pwszSave = StrDup(psz);
#endif
    if (pwszSave) {
        if (DPA_AppendPtr(g_hdpaCal, pwszSave) >= 0) {
            return TRUE;
        }
        LocalFree(pwszSave);
    }

    //
    //  Out of memory.  Bail.
    //
    MCDPADestroy(g_hdpaCal);
    g_hdpaCal = NULL;
    return FALSE;
}

HDPA MCGetCalInfoDPA(CALID calid, CALTYPE calType)
{
    HDPA hdpa = DPA_Create(4);

    ENTERCRITICAL;
    ASSERT(g_hdpaCal == NULL);
    g_hdpaCal = hdpa;
#ifdef WINNT
    EnumCalendarInfoW(MCEnumCalInfoProc, LOCALE_USER_DEFAULT, calid, calType);
#else
    EnumCalendarInfoA(MCEnumCalInfoProc, LOCALE_USER_DEFAULT, calid, calType);
#endif
    hdpa = g_hdpaCal;
    g_hdpaCal = NULL;
    LEAVECRITICAL;

    return hdpa;
}

void MCFreeCalendarInfo(PCALENDARTYPE pct)
{
    MCDPADestroy(pct->hdpaYears);
    MCDPADestroy(pct->hdpaEras);
    pct->hdpaYears = 0;
    pct->hdpaEras = 0;
}

//
//  Get all the era info and validate it so we don't fault when we try to
//  use them.
//
BOOL MCGetEraInfo(PCALENDARTYPE pct)
{
    int i;

    pct->hdpaYears = MCGetCalInfoDPA(pct->calid, CAL_IYEAROFFSETRANGE);
    if (!pct->hdpaYears)
        goto Bad;

    pct->hdpaEras = MCGetCalInfoDPA(pct->calid, CAL_SERASTRING);
    if (!pct->hdpaEras)
        goto Bad;

    // There must be at least one era...
    if (!DPA_GetPtrCount(pct->hdpaEras))
        goto Bad;

    // The number of eras must be equal to the number of era names
    if (DPA_GetPtrCount(pct->hdpaEras) != DPA_GetPtrCount(pct->hdpaYears))
        goto Bad;

    // The era dates must be in descending order.
    for (i = 1; i < DPA_GetPtrCount(pct->hdpaYears); i++)
    {
        if (StrToInt(DPA_FastGetPtr(pct->hdpaYears, i)) >
            StrToInt(DPA_FastGetPtr(pct->hdpaYears, i - 1)))
            goto Bad;
    }
    return TRUE;

Bad:
    /*
     *  Something went wrong, so clean up.
     */
    MCFreeCalendarInfo(pct);
    return FALSE;
}


//
// Check to see if this calendar is not supported currently
//
// Return FALSE for Hijri, Hebrew calendars, since these are
// Lunar calndars. This is hack so that this control behaves well when the calendar
// is any of the non-supported till we add this support to this control. [samera]
//
void MCGetCalendarInfo(PCALENDARTYPE pct)
{
    TCHAR tchCalendar[32];
    CALTYPE defCalendar = CAL_GREGORIAN;

    if (GetLocaleInfo(LOCALE_USER_DEFAULT,
                      LOCALE_ICALENDARTYPE,
                      tchCalendar,
                      ARRAYSIZE(tchCalendar)))
    {
        defCalendar = StrToInt(tchCalendar);
    }


    //
    //  Start with a clean slate.  Assume we don't have to do funky
    //  offset stuff (dyrOFfset = 0) or era stuff (hdpaEras = NULL),
    //  and that we don't need to do locale munging (LOCALE_USER_DEFAULT).
    //
    MCFreeCalendarInfo(pct);
    ZeroMemory(pct, sizeof(CALTYPE));
    pct->calid = defCalendar;
    pct->lcid = LOCALE_USER_DEFAULT;

    switch (pct->calid) {
    case CAL_GREGORIAN:
    case CAL_GREGORIAN_US:
    case CAL_GREGORIAN_ME_FRENCH:
    case CAL_GREGORIAN_ARABIC:
    case CAL_GREGORIAN_XLIT_ENGLISH:
    case CAL_GREGORIAN_XLIT_FRENCH:
        break;                          // Gregorian calendars are just fine

    case CAL_JAPAN:
    case CAL_TAIWAN:
        //
        //  These are era calendars.  Go get the era info.  Get hdpaEras
        //  last so we can use it to test whether we have a supported era
        //  calendar.
        //
        // If not enough memory to support traditional calendar, then just
        // force Gregorian.  Hey, at least we display *something*.
        //
        if (!MCGetEraInfo(pct))
            goto ForceGregorian;
        break;

    case CAL_THAI:
        pct->dyrOffset = BUDDHIST_BIAS; // You Just Have To Know this number
        break;

    case CAL_KOREA:
        pct->dyrOffset = KOREAN_BIAS;   // You Just Have To Know this number
        break;

    default:
        //
        // If the calenday isn't supported, then treat it as Gregorian. [samera]
        //
    ForceGregorian:
        pct->calid = CAL_GREGORIAN;
        pct->lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
        break;
    }

}

//
// Check whether the date string returned accorind to the
// current user-locale and calendar setting is Right-To-Left (RTL),
// and is so the RECTs for month year (@LBUTTONDOWN and NCHITTEST)
// needs to be adjusted. [samera]
//
BOOL MCIsDateStringRTL(TCHAR tch)
{
    WORD wAttrib=0;
    LCID lcidUserDefault;
    BOOL fRTL = FALSE;

    lcidUserDefault = GetUserDefaultLCID();

    if (lcidUserDefault)
    {
        //
        // Return TRUE if the 1st character is a RTL string.
        // A RTL char followed by a european number will
        // display visually as "european-num RTL-string" since the
        // BiDi layout algorithm of the language-pack will do
        // this. [samera]
        //
        if(GetStringTypeEx(lcidUserDefault,
                           CT_CTYPE2,
                           &tch,
                           1,
                           &wAttrib))
        {
            if(C2_RIGHTTOLEFT == wAttrib)
            {
                fRTL = TRUE;
            }

        }
    }

    return fRTL;
}

////////////////////////////////////////////////////////////////////////////
//
// Date/Time Picker
//
////////////////////////////////////////////////////////////////////////////

//
//  Subedit wrapper for the various weird things we need to get from NLS.
//
//  SE_YEARALT means that the year field is only two digits wide
//  in the format string, so we need to perform special Y2K enhancements.
//  For these fields, the field is displayed in two-digit format, but
//  when you go to edit the field, it temporarily changes to four-digit
//  format so you can change the century too.  Then when you complete the
//  edit, it returns to two-digit format.
//
//  The SE_YEARLIKE macro detects either SE_YEAR or SE_YEARALT.
//
//  SE_MONTHALT is just like SE_MONTH except that it is used
//  when the day (dd) comes before the month (mmm).  This is
//  important for languages like Russian where "12 October"
//  and "October 12" use different strings for the word
//  "October".  There is no way to get the alternate string
//  directly, except by creating a bogus date format that also
//  has the day before the month, then throwing away the day.
//
//  For example, if the incoming date string is
//
//      "MMMM dd yyyy"
//
//  we break it up into
//
//      "MMMM"      SE_MONTH
//      " "         SE_STATIC
//      "dd"        SE_DAY
//      " "         SE_STATIC
//      "yyyy"      SE_YEAR
//
//  However, if the incoming date is
//
//      "dd MMMM yyyy"
//
//  we break it up into
//
//      "dd"        SE_DAY
//      " "         SE_STATIC
//      "ddMMMM"    SE_MONTHALT
//      " "         SE_STATIC
//      "yyyy"      SE_YEAR
//
//  The extra "dd" at the beginning of SE_MONTHALT is stripped out below.
//
//  Y2K weirdness:  If we are getting the date format for the year,
//  and the year is being edited (either due to a SUBEDIT_ALL or because
//  it is the active subedit), and the format year is only
//  two digits, then force a four-digit year for editing purposes.
//
//
void SEGetTimeDateFormat(LPSUBEDIT pse, LPSUBEDITCONTROL psec, LPTSTR pszBuf, DWORD cchBuf)
{
    int cch;

    ASSERT(cchBuf >= 2);    // We assume it can hold at least space and a null
    pszBuf[0] = TEXT('\0');             // In case something fails

    if (pse->id == SE_MONTHALT) {
        TCHAR tszBuf[DTP_FORMATLENGTH + 3];
        //
        //  When we parsed the date string and realized that we needed
        //  the Alternate Month format, we created a date format of
        //  the form "ddMMM...", where "MMM..." is the
        //  the original month format.  Here we strip off the digits.
        //
        cch = GetDateFormat(psec->ct.lcid, 0, &psec->st, pse->pv, tszBuf, ARRAYSIZE(tszBuf));
        if (cch >= 2) {
            // [msadek] For Hebrew calander, day format "dd" is actually
            // two OR THREE characters. Don't hardcode it as 2.
            int cchDay = GetDateFormat(psec->ct.lcid, 0, &psec->st, TEXT("dd"), NULL, 0);
            lstrcpyn(pszBuf, tszBuf + cchDay - 1, cchBuf);
        }
    } else if (pse->id == SE_YEARALT &&
               (psec->iseCur == SUBEDIT_ALL || pse == &psec->pse[psec->iseCur])) {
        GetDateFormat(psec->ct.lcid, 0, &psec->st, TEXT("yyyy"), pszBuf, cchBuf);
    } else if (SE_DATELIKE(pse->id)) {
        GetDateFormat(psec->ct.lcid, 0, &psec->st, pse->pv, pszBuf, cchBuf);

        // Change a blank era to a space so the user can see it
        if (pse->id == SE_ERA && pszBuf[0] == TEXT('\0')) {
            pszBuf[0] = TEXT(' ');
            pszBuf[1] = TEXT('\0');
        }

    } else if (pse->id != SE_APP) {
        GetTimeFormat(LOCALE_USER_DEFAULT, 0, &psec->st, pse->pv, pszBuf, cchBuf);
    } else {

        NMDATETIMEFORMAT nmdtf = { 0 };
        nmdtf.pszFormat  = pse->pv;
        SECGetSystemtime(psec, &nmdtf.st);
        nmdtf.pszDisplay = nmdtf.szDisplay;

        CCSendNotify(psec->pci, DTN_FORMAT, &nmdtf.nmhdr);

        lstrcpyn(pszBuf, nmdtf.pszDisplay, cchBuf);

#ifdef UNICODE
        //
        // If the parent is an ANSI window, and pszDisplay
        // does not equal szDisplay, the then thunk had to
        // allocated memory for pszDisplay.  We need to
        // free it here.
        //

        if (!psec->pci->bUnicode && nmdtf.pszDisplay &&
            nmdtf.pszDisplay != nmdtf.szDisplay) {
            LocalFree ((LPSTR)nmdtf.pszDisplay);
        }
#endif
    }
}

//
// SUBEDIT stuff for DateTimePicker
//
// NOTE: Now that the DatePicker and TimePicker are combined,
// this could be moved back into the parent structure.
//

//
//  Used in an era calendar to get the length of the longest era name.
//  Also leaves a random heigth in psize->cy because that's what the
//  non-era code does, too.
//
int SECGetMaxEraLength(PCALENDARTYPE pct, HDC hdc, PSIZE psize)
{
    int i;
    int wid = 0;
    for (i = 0; i < DPA_GetPtrCount(pct->hdpaEras); i++)
    {
        LPCTSTR ptsz = DPA_FastGetPtr(pct->hdpaEras, i);
        if (GetTextExtentPoint32(hdc, ptsz, lstrlen(ptsz), psize) &&
            psize->cx > wid)
        {
            wid = psize->cx;
        }
    }
    return wid;
}


// SECRecomputeSizing needs to calculate the maximum rectangle each subedit can be. Ugh.
//
// The size of the SE_YEARALT field changes depending on whether or not it is
// the current psec->iseCur.  Double ugh.

void SECRecomputeSizing(LPSUBEDITCONTROL psec, LPRECT prc)
{
    HDC       hdc;
    HGDIOBJ   hfontOrig;
    int       i;
    LPSUBEDIT pse;
    int       left = prc->left;

    psec->rc = *prc;

    hdc       = GetDC(psec->pci->hwnd);
    hfontOrig = SelectObject(hdc, (HGDIOBJ)psec->hfont);

    for (i=0, pse=psec->pse; i < psec->cse ; i++, pse++)
    {
        TCHAR   szTmp[DTP_FORMATLENGTH];
        LPCTSTR sz;
        int     min, max;
        SIZE    size;
        int     wid;

        min = pse->min;
        max = pse->max;
        if (pse->id == SE_STATIC)
        {
            ASSERT(pse->fReadOnly);
            sz = pse->pv;
        }
        else
        {
            sz = szTmp;

            // make some assumptions so we don't loop more than we have to
            switch (pse->id)
            {
                
            // we only need seven for the text days of the week
            case SE_DAY:
                min = 10; // make them all double-digit
                max = 17;
                break;

            // Assume we only have numeric output with all chars same width
            case SE_MARK:
                min = 11;
                max = 12;
                break;

            case SE_HOUR:
            case SE_YEAR:
            case SE_YEARALT:
            case SE_MINUTE:
            case SE_SECOND:
                min = max;
                break;

            case SE_ERA:
                if (ISERACALENDAR(&psec->ct)) {
                    wid = SECGetMaxEraLength(&psec->ct, hdc, &size);
                    goto HaveWidth;
                } else {
                    min = max = *pse->pval; // current value is good enough
                }
                break;
            }
        }

        // now get max width
        if (pse->id == SE_APP)
        {
            NMDATETIMEFORMATQUERY nmdtfq = {0};

            nmdtfq.pszFormat = pse->pv;

            CCSendNotify(psec->pci, DTN_FORMATQUERY, &nmdtfq.nmhdr);

            size = nmdtfq.szMax;
            wid  = nmdtfq.szMax.cx;
        }
        else
        {
            SYSTEMTIME st = psec->st;

            /*
             *  SUBTLE - Munge the month/day to January 1.  This solves
             *  lots of problems, such as "Today is Feb 29 1996, and
             *  when we iterate through year = 1997, we get Feb 29 1997,
             *  which is invalid."   Or "Today is Jan 31 1999, and when
             *  we iterate through the months, we get Sep 31 1999, which
             *  is invalid."
             *
             *  We choose "January 1" because
             *
             *  1. Every year has a "January 1", so the year can vary.
             *  2. Every month has a "first", so the month can vary.
             *  3. Every day up to 31 is valid in January, so the day can vary.
             */
            psec->st.wMonth = psec->st.wDay = 1;

            for (wid = 0 ; min <= max ; min++)
            {
                if (pse->id != SE_STATIC)
                {
                    *pse->pval = (WORD) min;

                    SEGetTimeDateFormat(pse, psec, szTmp, ARRAYSIZE(szTmp));
                    if (szTmp[0] == TEXT('\0'))
                    {
                        DebugMsg(TF_ERROR, TEXT("SECRecomputeSizing: GetDate/TimeFormat([%s] y=%d m=%d d=%d h=%d m=%d s=%d) = ERROR %d"),
                            pse->pv, psec->st.wYear, psec->st.wMonth, psec->st.wDay, psec->st.wHour, psec->st.wMinute, psec->st.wSecond,
                            GetLastError());

                    }
                }
                if (!GetTextExtentPoint32(hdc, sz, lstrlen(sz), &size))
                {
                    size.cx = 0;
                    DebugMsg(TF_MONTHCAL,TEXT("SECRecomputeSizing: GetTextExtentPoint32(%s) = ERROR %d"), sz, GetLastError());
                }
                if (size.cx > wid)
                    wid = size.cx;
            }
            psec->st = st;
        }
HaveWidth:
        // now set up subedit's bounding rectangle
        pse->rc.top    = prc->top + SECYBORDER;
        pse->rc.bottom = pse->rc.top + size.cy;
        pse->rc.left   = left;
        pse->rc.right  = left + wid;
        left = pse->rc.right;
    }

    SelectObject(hdc, hfontOrig);
    ReleaseDC(psec->pci->hwnd, hdc);
}

// InitSubEditControl parses szFormat into psec, setting the time to pst.
TCHAR c_szFormats[] = TEXT("gyMdthHmsX");
BOOL PASCAL SECParseFormat(DATEPICK* pdp, LPSUBEDITCONTROL psec, LPCTSTR szFormat)
{
    LPCTSTR   pFmt;
    LPTSTR    psecFmt;
    int       cse, cchExtra;
    int       nTmp;
    LPSUBEDIT pse;
    BOOL      fDaySeen = FALSE;
    BOOL      fForceCentury = FALSE;
    int       iLen, i;
    TCHAR     tch;
    LPTSTR    pFmtTemp;
    TCHAR szFormatTemp[DTP_FORMATLENGTH];
    
    //
    //  We need to force the century if the format is
    //  DTS_SHORTDATECENTURYFORMAT.
    //
    if (pdp->fLocale &&
        (pdp->ci.style & DTS_FORMATMASK) == DTS_SHORTDATECENTURYFORMAT)
    {
        fForceCentury = TRUE;
    }

    // [msadek]; If we need to mirror the format and the
    // cleint passed a read only buffer, we will AV (W2k bug# 354533)
    // Let's copy it first

    if (psec->fMirrorSEC)
    {
        lstrcpyn(szFormatTemp, szFormat, ARRAYSIZE(szFormatTemp));
        szFormat = szFormatTemp;
    }
 
    // count szFormat sections so we know what to allocate
    pFmt = szFormat;
    cse = 0;
    cchExtra = 0;
    while (*pFmt)
    {
        if (StrChr(c_szFormats, *pFmt)) // format string
        {
            TCHAR c = *pFmt;
            while (c == *pFmt)
                pFmt++;
            cse++;

            // If it was a string Month format, reserve 2 more chars for
            // the possible "dd" leader, in case we need SE_MONTHALT.
            if (c == TEXT('M'))
                cchExtra += 2;

        }
        else if (*pFmt == TEXT('\'')) // quoted static string
        {
KeepSearching:
            pFmt++;
            while (*pFmt && *pFmt != TEXT('\''))
                pFmt++;
            if (*pFmt) // handle poorly quoted strings
            {
                pFmt++;
                if (*pFmt == TEXT('\'')) // quoted quote, not end of quote
                    goto KeepSearching;
            }
            cse++;
        }
        else // static string probably a delimiter
        {
            while (*pFmt && *pFmt!=TEXT('\'') && !StrChr(c_szFormats, *pFmt))
                pFmt++;
            cse++;
        }
    }

    // Allocate space
    nTmp = cse + lstrlen(szFormat) + cchExtra + 1; // number of chars
    nTmp = nTmp * sizeof(TCHAR); // size in BYTES
    nTmp = (nTmp + 3) & (~3); // round up to DWORD boundary (MIPS alignment issue)
    psecFmt = (LPTSTR)LocalAlloc(LPTR, nTmp + cse * sizeof(SUBEDIT));
    if (!psecFmt)
    {
        DebugMsg(TF_MONTHCAL, TEXT("SECParseFormat failed to allocate memory"));
        return FALSE; // use whatever we already have
    }
    
    if (psec->szFormat)
        LocalFree(psec->szFormat);
    
    psec->szFormat   = psecFmt;
    psec->cDelimeter = '\0';
    psec->pse        = (LPSUBEDIT)((LPBYTE)psecFmt + nTmp);

    // Fill psec
    psec->iseCur = SUBEDIT_NONE;
    psec->cse    = cse;
    pse          = psec->pse;
    ZeroMemory(pse, cse * sizeof(SUBEDIT));
    pFmt = szFormat;
    pdp->fHasMark = FALSE;


    //
    // Before start parsing the format string, let's mirror it if requested. 
    //
    if (psec->fMirrorSEC)
    {
        pFmtTemp = (LPTSTR)pFmt;
        iLen = lstrlen(pFmtTemp);
        for( i=0 ; i<iLen/2 ; i++ )
        {
            tch = pFmtTemp[i];
            pFmtTemp[i] = pFmtTemp[iLen-i-1];
            pFmtTemp[iLen-i-1] = tch;
        }
    }


    while (*pFmt)
    {
        pse->flDrawText = DT_CENTER;

        if (*pFmt == TEXT('y') || *pFmt == TEXT('g')) // y=year g=era
        {
            TCHAR ch = *pFmt;

            // If the calendar doesn't use eras, then the era field is just
            // for show and can't be changed.
            if (ch == TEXT('g') && !ISERACALENDAR(&psec->ct)) {
                pse->fReadOnly = TRUE;
            }

            pse->id     = ch == TEXT('y') ? SE_YEAR : SE_ERA;
            pse->pval   = &psec->st.wYear;
            pse->min    = c_stEpoch.wYear;
            pse->max    = c_stArmageddon.wYear;
            pse->cchMax = 0;
            
            pse->pv = psecFmt;
            while (*pFmt == ch) {
                pse->cchMax++;
                *psecFmt++ = *pFmt++;
            }

            if (pse->id == SE_YEAR)
            {
                pse->flDrawText = DT_RIGHT;

                if (fForceCentury)
                {
                    pse->pv = TEXT("yyyy");
                    pse->cchMax = 4;
                }
                else
                {
                    switch (pse->cchMax)
                    {
                    case 1:                 //  "y" is a SE_YEARALT
                    case 2:                 // "yy" is a SE_YEARALT
                        pse->id = SE_YEARALT;
                        pse->cchMax = 4;    // Force four-digit editing
                        break;

                    case 3:                 // "yyy" is an alias for "yyyy".
                        pse->cchMax = 4;
                        break;
                    }
                }
            }

            *psecFmt++ = TEXT('\0');
        }
        else if (*pFmt == TEXT('M')) // month
        {
            pse->pv = psecFmt;

            // If the day has been seen, then we need to use the alternate
            // month format, so set up the gratuitous "dd" prefix.
            // See SEGetTimeDateFormat.
            //
            if (fDaySeen) {
                pse->id = SE_MONTHALT;
                *psecFmt++ = TEXT('d');
                *psecFmt++ = TEXT('d');
            } else {
                pse->id = SE_MONTH;
            }
            pse->pval   = &psec->st.wMonth;
            pse->min    = 1;
            pse->max    = 12;
            pse->cchMax = 2;

            while (*pFmt == TEXT('M'))
                *psecFmt++ = *pFmt++;
            if (psecFmt - pse->pv <= 2)
                pse->flDrawText = DT_RIGHT;
            *psecFmt++ = TEXT('\0');
        }
        
#if 0 // if we ever do week-of-year format
        else if (*pFmt == TEXT('w')) // week
        {
            // BUGBUG: NLS date functions can modify the displayed YEAR if the displayed WEEK overflows
            // into the next or prior year!
            // BUGBUG: We need to make the week increment move beyond month boundries...
            // In order to do this, we need to add an "overflow" flag so day increments will
            // carry into month increments etc.

            pse->id         = SE_DAY;
            pse->pval       = &psec->st.wDay;
            pse->min        = 1;
            pse->max        = GetDaysForMonth(psec->st.wYear, psec->st.wMonth);
            pse->cIncrement = 7;
            pse->cchMax     = 2;

            pse->pv = psecFmt;
            while (*pFmt == TEXT('w'))
                *psecFmt++ = *pFmt++;
            *psecFmt++ = TEXT('\0');
        }
#endif
        else if (*pFmt == TEXT('d')) // day or day of week
        {
            fDaySeen    = TRUE;     // See SEGetTimeDateFormat
            pse->id     = SE_DAY;
            pse->pval   = &psec->st.wDay;
            pse->min    = 1;
            pse->max    = GetDaysForMonth(psec->st.wYear, psec->st.wMonth);
            pse->cchMax = 2;

            pse->pv = psecFmt;
            while (*pFmt == TEXT('d'))
                *psecFmt++ = *pFmt++;
            if (psecFmt - pse->pv <= 2)
                pse->flDrawText = DT_RIGHT;     // day
            else
                pse->fReadOnly = TRUE;          // day of week
            *psecFmt++ = TEXT('\0');

        }
        else if (*pFmt == TEXT('t')) // marker
        {
            pdp->fHasMark = TRUE;
            pse->id         = SE_MARK;
            pse->pval       = &psec->st.wHour;
            pse->min        = 0;
            pse->max        = 23;
            pse->cIncrement = 12;
            pse->cchMax     = 2;

            pse->pv = psecFmt;
            while (*pFmt == TEXT('t'))
                *psecFmt++ = *pFmt++;
            *psecFmt++ = TEXT('\0');
        }
        else if (*pFmt == TEXT('h')) // (12) hour
        {
            pse->id     = SE_HOUR;
            pse->pval   = &psec->st.wHour;
            pse->min    = 0;
            pse->max    = 23;
            pse->cchMax = 2;
            pse->flDrawText = DT_RIGHT;

            pse->pv = psecFmt;
            while (*pFmt == TEXT('h'))
                *psecFmt++ = *pFmt++;
            *psecFmt++ = TEXT('\0');

        }
        else if (*pFmt == TEXT('H')) // (24) hour
        {
            pse->id     = SE_HOUR;
            pse->pval   = &psec->st.wHour;
            pse->min    = 0;
            pse->max    = 23;
            pse->cchMax = 2;
            pse->flDrawText = DT_RIGHT;

            pse->pv = psecFmt;
            while (*pFmt == TEXT('H'))
                *psecFmt++ = *pFmt++;
            *psecFmt++ = TEXT('\0');
        }
        else if (*pFmt == TEXT('m')) // minute
        {
            pse->id     = SE_MINUTE;
            pse->pval   = &psec->st.wMinute;
            pse->min    = 0;
            pse->max    = 59;
            pse->cchMax = 2;
            pse->flDrawText = DT_RIGHT;

            pse->pv = psecFmt;
            while (*pFmt == TEXT('m'))
                *psecFmt++ = *pFmt++;
            *psecFmt++ = TEXT('\0');
        }
        else if (*pFmt == TEXT('s')) // second
        {
            pse->id     = SE_SECOND;
            pse->pval   = &psec->st.wSecond;
            pse->min    = 0;
            pse->max    = 59;
            pse->cchMax = 2;
            pse->flDrawText = DT_RIGHT;

            pse->pv = psecFmt;
            while (*pFmt == TEXT('s'))
                *psecFmt++ = *pFmt++;
            *psecFmt++ = TEXT('\0');
        }
        else if (*pFmt == TEXT('X')) // app specified field
        {
            pse->id = SE_APP;

            pse->pv = psecFmt;
            while (*pFmt == TEXT('X'))
                *psecFmt++ = *pFmt++;
            *psecFmt++ = TEXT('\0');
        }
        else if (*pFmt == TEXT('\'')) // quoted static string
        {
            pse->id      = SE_STATIC;
            pse->fReadOnly = TRUE;

            pse->pv = psecFmt;
SearchSomeMore:
            pFmt++;
            while (*pFmt && *pFmt != TEXT('\''))
                *psecFmt++ = *pFmt++;
            if (*pFmt) // handle poorly quoted strings
            {
                pFmt++;
                if (*pFmt == TEXT('\'')) // quoted quote, not end of quote
                {
                    *psecFmt++ = *pFmt;
                    goto SearchSomeMore;
                }
            }
            *psecFmt++ = TEXT('\0');
        }
        else // unknown non-editable stuff (most likely a delimeter)
        {
            // BUGBUG: even though it's unknown, we should probably pass
            // it off to GetDateFormat so we will be forward compatible
            // with future date formats...
            //
            pse->id      = SE_STATIC;
            pse->fReadOnly = TRUE;

            if (!psec->cDelimeter)
                psec->cDelimeter = *pFmt;

            pse->pv = psecFmt;
            while (*pFmt && *pFmt!=TEXT('\'') && !StrChr(c_szFormats, *pFmt))
                *psecFmt++ = *pFmt++;
            *psecFmt++ = TEXT('\0');

            // we'll assume that the first not formatting char is the
            // delimeter...maybe not a great assumption, but it will work
            // most of the time.
            //
        }
        pse++;
    }

#ifdef DEBUG
{
    TCHAR sz[200];
    LPTSTR psz;
    psz = sz;
    sz[0]=TEXT('\0');
    pse = psec->pse;
    cse = psec->cse;
    while (cse > 0)
    {
        wsprintf(psz, TEXT("[%s] "), pse->pv);
        psz = psz + lstrlen(psz);
        cse--;
        pse++;
    }
    DebugMsg(TF_MONTHCAL, TEXT("SECParseFormat: %s"), sz);
}
#endif

    //
    // Let restore the original format
    //
    if (psec->fMirrorSEC)
    {
        pFmtTemp = (LPTSTR)szFormat;
        for( i=0 ; i<iLen/2 ; i++ )
        {
            tch = pFmtTemp[i];
            pFmtTemp[i] = pFmtTemp[iLen-i-1];
            pFmtTemp[iLen-i-1] = tch;
        }
    }


    //
    // If this is a time-only DTP control and we need to swap the AM/PM symbol
    // to the other side, then let's do it.
    //
    if (psec->fSwapTimeMarker)
    {
        SUBEDIT se;
        pse = psec->pse;
        cse = psec->cse;

        if ((cse > 1) && (psec->pse[0].id == SE_MARK))
        {
            se = psec->pse[0];
            i = 0;
            while( i < (cse-1) )
            {
                pse[i] = pse[i+1];
                i++;
            }
            pse[psec->cse-1] = se;
        }
    }

    // The subedits have changed, recompute sizes
    SECRecomputeSizing(psec, &psec->rc);

    // We're going to need to redraw this
    InvalidateRect(psec->pci->hwnd, NULL, TRUE);

    // Changing the format also changes the window text.
    MyNotifyWinEvent(EVENT_OBJECT_NAMECHANGE, pdp->ci.hwnd, OBJID_WINDOW, INDEXID_CONTAINER);
    return TRUE;
}

void SECDestroy(LPSUBEDITCONTROL psec)
{
    if (psec->szFormat)
    {
        LocalFree(psec->szFormat);
        psec->szFormat = NULL;
    }
}
void SECSetFont(LPSUBEDITCONTROL psec, HFONT hfont)
{
    if (hfont == NULL)
        hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    psec->hfont = hfont;
}

void InvalidateScrollRect(HWND hwnd, RECT *prc, int xScroll)
{
    RECT rc;

    if (xScroll)
    {
        rc = *prc;
        OffsetRect(&rc, -xScroll, 0);
        prc = &rc;
    }
    InvalidateRect(hwnd, prc, TRUE);
}

void SECSaveResetSubeditEdit(DATEPICK *pdp, BOOL fReset);
#define SECSaveSubeditEdit(pdp) SECSaveResetSubeditEdit(pdp, FALSE)
#define SECResetSubeditEdit(pdp) SECSaveResetSubeditEdit(pdp, TRUE)

// Set the current subedit, scrolling things into view as needed
void SECSetCurSubed(DATEPICK *pdp, int isubed)
{
    LPSUBEDITCONTROL psec = &pdp->sec;

    // validate the arguments
    ASSERT(isubed < psec->cse);
    
    // if the subedit is changing, we need to invalidate stuff
    if (isubed != psec->iseCur)
    {
        int isePre;
        if (psec->iseCur >= 0)
        {
            SECResetSubeditEdit(pdp);
            InvalidateScrollRect(psec->pci->hwnd, &psec->pse[psec->iseCur].rc, psec->xScroll);
        }

        isePre = psec->iseCur;
        psec->iseCur = isubed;
        // For perf reasons, do a full recompute only if SE_YEARALT or
        // SUBEDIT_ALL was involved, since those are the only cases
        // where SE_YEARALT fields change size.

        #define YearAffected(psec, ise)                             \
                (ise == SUBEDIT_ALL ||                              \
                 (ise >= 0 && psec->pse[ise].id == SE_YEARALT))

        if (YearAffected(psec, isePre) || YearAffected(psec, isubed))
        {
            SECRecomputeSizing(psec, &psec->rc);
            InvalidateRect(psec->pci->hwnd, NULL, TRUE);
        }
        #undef YearAffected

        if (psec->iseCur >= 0)
        {
            RECT rc = psec->pse[psec->iseCur].rc;
            OffsetRect(&rc, -psec->xScroll, 0);
            if (rc.left < psec->rc.left)
            {
                psec->xScroll += rc.left - psec->rc.left;
                InvalidateRect(psec->pci->hwnd, NULL, TRUE);
            }
            else if (rc.right > psec->rc.right)
            {
                psec->xScroll += rc.right - psec->rc.right;
                InvalidateRect(psec->pci->hwnd, NULL, TRUE);
            }
            else
            {
                InvalidateRect(psec->pci->hwnd, &rc, TRUE);
            }
        }
    }
}

int SECIncrFocus(DATEPICK *pdp, int delta)
{
    int ise, loop;
    LPSUBEDITCONTROL psec = &pdp->sec;

    ASSERT(-1 == delta || 1 == delta);  

    ise = psec->iseCur;
    if (ise < 0 && delta < 0)
        ise = psec->cse;

    for (loop = 0 ; loop < psec->cse ; loop++)
    {
        int oldise = ise;
        ise = (ise + delta + psec->cse) % psec->cse;
        if (ise != oldise+delta && psec->fNone)
        {
            // we wrapped and we allow scrolling into SUBEDIT_NONE state
            break;
        }
        if (!psec->pse[ise].fReadOnly)
        {
            goto Found;
        }
    }
    ise = SUBEDIT_NONE;
Found:
    SECSetCurSubed(pdp, ise);
    return ise;
}

void SECInvalidate(LPSUBEDITCONTROL psec, int id);

//
//  Given a Gregorian year, get the local name for that year.
//
UINT SECGetYearValue(DATEPICK *pdp, UINT uYear)
{
    UINT uiRc = 0;
    TCHAR rgch[64];
    if (EVAL(MCGetDateFormatWithTempYear(&pdp->sec.ct, &pdp->sec.st, TEXT("yyyy"), uYear, rgch, ARRAYSIZE(rgch)))) {
        uiRc = StrToInt(rgch);
    }
    return uiRc;
}

//
//  SECAdjustByEra
//
//  pct - PCALENDARTYPE structure to use for conversion
//
//  uInput - the value the user typed (to be interpreted as local calendar)
//
//  The basic idea is that if you type a year, it is interpreted relative
//  to the era you were in previously.  If the number you typed isn't valid
//  for that era, then reject it (by returning the original year unchanged).
//
UINT SECAdjustByEra(DATEPICK *pdp, UINT uInput)
{
    UINT uResult = pdp->sec.st.wYear;

    //
    //  Find the delta between the current local year and the current
    //  Gregorian year.  We don't use any of the era transition dates
    //  since they aren't reliable at the boundaries.  Just convert it
    //  to a display name and re-parse it back.
    //
    UINT uDelta = pdp->sec.st.wYear - SECGetYearValue(pdp, pdp->sec.st.wYear);

    //
    //  Apply that delta to the year the user typed in.  This converts the
    //  local year into a Gregorian year.
    //
    UINT uNewVal = uInput + uDelta;

    // uNewVal is the value we want to change it to.  If it's valid for that
    // era, then use it.  We detect that it's okay for the era by converting
    // it to a display name and seeing if it matches.  It can fail for being
    // too large (past the end of the era) or too small (trying to change to
    // January 1 local year 1 when the era didn't change until March).

    if (SECGetYearValue(pdp, uNewVal) == uInput) {
        uResult = uNewVal;
    }

    return uResult;
}

//
//  SECAdjustByType
//
//  Some field types are special.
//
//  SE_YEAR or SE_YEARALT if user typed only two digits
//
//      Use the "implied century for two-digit years" logic
//      as described in NT5_GetCalendarInfoA.
//
//  SE_YEAR or SE_YEARALT if user typed more than two digits
//
//      Use that number.
//
//  BONUS FEATURE!
//
//      Some calendars run parallel to the Gregorian year,
//      but with different years.  Use GregorianToOther and
//      OtherToGregorian to convert.
//
//      The input value is the local year (Gregorian, Buddhist, whatever)
//      but the return value is always the Gregorian year, since that's
//      what SYSTEMTIME uses.
//
//  SE_HOUR
//
//      If the clock is in 12-hour format, then preserve AM/PM ness of
//      the hour.  For example, if it was 3pm and somebody is changing
//      the hour to 4, use 4pm instead of 4am.
//
UINT SECAdjustByType(DATEPICK *pdp, LPSUBEDIT psubed, UINT uNewValue)
{

    if (SE_YEARLIKE(psubed->id))
    {
        if (uNewValue < 100)
        {
            // Get the preferred century of the preferred calendar
            // (in the localized year, not Gregorian.)
            DWORD dwMax2DigitYear;
            if (!NT5_GetCalendarInfoA(pdp->sec.ct.lcid, pdp->sec.ct.calid, CAL_RETURN_NUMBER + CAL_ITWODIGITYEARMAX,
                                     NULL, 0, &dwMax2DigitYear))
            {
                // default in the absence of all information
                dwMax2DigitYear = GregorianToOther(&pdp->sec.ct, 2029);

                // if the current year in this era is less than 100, then the 2 digits typed
                // may be the real date, so set the max to 99 (i.e., no conversion)
                //
                if (dwMax2DigitYear < 99)
                    dwMax2DigitYear = 99;
            }

            //
            //  Copy the century of dwMax2DigitYear into uNewValue.
            //
            uNewValue += (dwMax2DigitYear - dwMax2DigitYear % 100);
            //
            //  If it exceeds the max, then drop to previous century.
            //
            if (uNewValue > dwMax2DigitYear)
                uNewValue -= 100;

        }

        //
        //  Finally, convert back to Gregorian as necessary.
        //
        uNewValue = OtherToGregorian(&pdp->sec.ct, uNewValue);

        //
        //  If we are in an Era calendar, then we need to adjust the
        //  year relative to the ambient era.
        //
        if (ISERACALENDAR(&pdp->sec.ct)) {
            uNewValue = SECAdjustByEra(pdp, uNewValue);
        }

    } else if (psubed->id == SE_HOUR && psubed->pv[0] == TEXT('h')) {
        if (*psubed->pval >= 12 && uNewValue < 12)
            uNewValue += 12;
    }

    return uNewValue;
}


void SECSetSubeditValue(DATEPICK *pdp, LPSUBEDIT psubed, UINT uNewValue, BOOL fForce)
{
    LPSUBEDITCONTROL psec = &pdp->sec;
    UINT uOldValue;

    uNewValue = SECAdjustByType(pdp, psubed, uNewValue);

    //
    //  Must do a full-on range check in addition to the lame psubed->min
    //  psubed->max range check because the new value might be valid
    //  for our range but not in the global scheme of things.  For example,
    //  the minimum date is Sep 14 1752, but if today is Jan 1 1995 and
    //  the user types "1752", that will pass the simple min/max year test,
    //  but it's not a valid date since Jan 1 1752 is out of range.
    //

    uOldValue = *psubed->pval;
    *psubed->pval = (WORD)uNewValue;
    if (uNewValue >= psubed->min && uNewValue <= psubed->max &&
        CmpSystemtime(&pdp->sec.st, &pdp->stMin) >= 0 &&
        CmpSystemtime(&pdp->sec.st, &pdp->stMax) <= 0)
    {
        if (fForce || uNewValue != uOldValue)
        {
            SECInvalidate(psec, SE_APP);
            InvalidateScrollRect(psec->pci->hwnd, &psubed->rc, psec->xScroll);

            DPNotifyDateChange(pdp);
        }
    }
    else
    {
        // Oops, not valid, put the old value back
        *psubed->pval = (WORD)uOldValue;
    }
}

// This saves the current pending value and also resets the edit state
// if fReset is TRUE
void SECSaveResetSubeditEdit(DATEPICK *pdp, BOOL fReset)
{
    LPSUBEDITCONTROL psec = &pdp->sec;

    if (psec->iseCur >= 0)
    {
        LPSUBEDIT psubed = &psec->pse[psec->iseCur];

        if (psubed->cchEdit)
        {
            SECSetSubeditValue(pdp, psubed, psubed->valEdit, FALSE);
        }
        if (fReset)
            psubed->cchEdit = 0;
    }
}

// SECInvalidate invalidates the display for each subedit affected by a change
// to ID.  NOTE: as a side affect, it recalculates MAX fields for all subedits
// affected by a change to ID.
//
// SE_APP invalidates everything, anything invalidates SE_APP
// SE_MARK (am/pm) invalidate SE_HOUR, SE_HOUR invalides SE_MARK
// 
void SECInvalidate(LPSUBEDITCONTROL psec, int id)
{
    BOOL fAdjustDayMax = (id == SE_MONTH || id == SE_MONTHALT || id == SE_YEAR || id == SE_YEARALT || id == SE_APP || id == SE_ERA);
    LPSUBEDIT pse;
    int i;

    // If we changed any date field and we are in a era-like calendar,
    // then invalidate all, since changing the month, day or year may
    // change the era or vice versa.
    if (ISERACALENDAR(&psec->ct) && SE_DATELIKE(id))
    {
        id = SE_APP;
    }

    for (pse=psec->pse, i=0 ; i < psec->cse ; pse++, i++)
    {
        // we need to invalidate all fields that changed
        if (id == pse->id || pse->id == SE_APP || id == SE_APP || (id == SE_MARK && pse->id == SE_HOUR) || (id == SE_HOUR && pse->id == SE_MARK))
        {
            InvalidateScrollRect(psec->pci->hwnd, &pse->rc, psec->xScroll);
        }

        // the month or year changed, fix max field for SE_DAY
        if (fAdjustDayMax && pse->id == SE_DAY)
        {
            pse->max = GetDaysForMonth(psec->st.wYear, psec->st.wMonth);
            if (*pse->pval > pse->max)
            {
                *pse->pval = (WORD) pse->max;
            }
            SECInvalidate(psec, SE_DAY);
        }
    }
}

__inline
BOOL
SECGetEraName(LPSUBEDITCONTROL psec, LPSUBEDIT pse, UINT uYear, LPTSTR ptszBuf, UINT cchBuf)
{
    return MCGetDateFormatWithTempYear(&psec->ct, &psec->st, pse->pv, uYear, ptszBuf, cchBuf);
}

//
//  SECIncrementEra increments/decrements the era field.  ERAs are strange
//  since they aren't a field unto themselves but are rather an artifact
//  of the other fields.  Returns the new year to use.
//
UINT SECIncrementEra(LPSUBEDITCONTROL psec, LPSUBEDIT pse, int delta)
{
    TCHAR rgch[64];
    TCHAR rgch2[64];
    int i;
    int cEras = DPA_GetPtrCount(psec->ct.hdpaEras);

    UINT uNewYear;

    ASSERT(pse->pval == &psec->st.wYear);
    uNewYear = psec->st.wYear;

    //
    //  First find the era that encloses the current year.
    //  Do this by comparing the era string, because it's possible
    //  for the era to change twice within the same calendar year
    //  (if an emperor ascends to the throne and then dies the next week)
    //  so comparing against hdpaYear won't help.
    //
    SECGetEraName(psec, pse, uNewYear, rgch, ARRAYSIZE(rgch));

    //
    //  If the era string is blank, it means we're in the "before the
    //  first era" scenario, so we use the "virtual" last element that
    //  represents "minus infinity".
    //
    if (rgch[0] == TEXT('\0'))
    {
        i = cEras;
        goto FoundEra;
    }

    for (i = 0; i < cEras; i++)
    {
        if (lstrcmp(rgch, DPA_FastGetPtr(psec->ct.hdpaEras, i)) == 0)
            goto FoundEra;
    }

    //
    //  Eek!  Couldn't find the era!  Just increment/decrement the
    //  year instead.
    //
    uNewYear += delta;
    goto Finish;

FoundEra:

    //
    //  The era list is stored backwards, so incrementing the era means
    //  decrementing the index (i).
    //

    if (delta > 0) // Incrementing
    {
        //
        //  Don't go off the end of the list.  Note that if we were in
        //  the "virtual era" at minus infinity, this decrement will move
        //  us into the first "real" era.
        //
        if (--i < 0)
            goto Finish;

        //   Increment to first year of the next era.
        uNewYear = StrToInt(DPA_FastGetPtr(psec->ct.hdpaYears, i));
    }
    else
    {
        //
        //  Don't go off the end of the list.  Note that this also
        //  catches the "virtual era" at minus infinity.
        //
        if (i >= cEras)
            goto Finish;

        //
        //  Move to the last year of the previous era.  Do this by
        //  starting with the first year of the current era and
        //  decrementing it if necessary.
        //
        uNewYear = StrToInt(DPA_FastGetPtr(psec->ct.hdpaYears, i));
    }

    //
    //  We have a year that might be in the next/prev era.  Try it.
    //  If we're still in the original era, then inc/dec one more time
    //  to get there for good.
    //
    SECGetEraName(psec, pse, uNewYear, rgch2, ARRAYSIZE(rgch2));
    if (lstrcmp(rgch, rgch2) == 0)
        uNewYear += delta;

Finish:
    if (uNewYear < pse->min)
        uNewYear = pse->min;
    if (uNewYear > pse->max)
        uNewYear = pse->max;
    return uNewYear;
}

// SECIncrementSubedit increments currently selected subedit by delta
// Returns TRUE iff the value changed
BOOL SECIncrementSubedit(LPSUBEDITCONTROL psec, int delta)
{
    LPSUBEDIT psubed;
    UINT val;

    if (psec->iseCur < 0)
        return(FALSE);

    psubed = &psec->pse[psec->iseCur];

    if (psubed->id == SE_APP)
        return(FALSE);

    //
    //  Only numeric fields should accelerate.  Text fields should always
    //  increment/decrement by exactly one position.
    //
    if (psubed->flDrawText & DT_CENTER) {
        if (delta < 0) delta = -1;
        if (delta > 0) delta = +1;
    }

    //
    //  Incrementing/decrementing ERAs is strange.
    //
    if (psubed->id == SE_ERA)
    {
        val = SECIncrementEra(psec, psubed, delta);
    }
    else
    {
        // delta isn't a REAL delta -- it's a directional thing. Here's the REAL delta:
        if (psubed->cIncrement > 0)
            delta = delta * psubed->cIncrement;
        if(!psubed->pval)
            return (FALSE);
            
        val = *psubed->pval + delta;
        while (1) {
            if ((int)val < (int)psubed->min)
            {
                // don't wrap years
                if (SE_YEARLIKE(psubed->id)) {
                    val = psubed->min;
                    break;
                }
                val = psubed->min - val - 1;
                val = psubed->max - val;
            }
            else if (val > psubed->max)
            {
                // don't wrap years
                if (SE_YEARLIKE(psubed->id)) {
                    val = psubed->max;
                    break;
                }
                val = val - psubed->max - 1;
                val = psubed->min + val;
            } else
                break;
        }
    }

    if (*psubed->pval != val)
    {
        *psubed->pval = (WORD) val;

        SECInvalidate(psec, psubed->id);
        return(TRUE);
    }

    return(FALSE);
}

// returns TRUE if a value has changed, FALSE otherwise
BOOL SECHandleKeydown(DATEPICK *pdp, WPARAM wParam, LPARAM lParam)
{
    int delta = 1;
    LPSUBEDITCONTROL psec = &pdp->sec;

    switch (wParam)
    {
    case VK_LEFT:
        delta = -1;
        // fall through...
    case VK_RIGHT:
        SECResetSubeditEdit(pdp);
        SECIncrFocus(pdp, delta);
        return(FALSE);
    }

    if (psec->iseCur >= 0 &&
        psec->pse[psec->iseCur].id == SE_APP)
    {
        NMDATETIMEWMKEYDOWN nmdtkd = {0};

        nmdtkd.nVirtKey  = (int) wParam;
        nmdtkd.pszFormat = psec->pse[psec->iseCur].pv;
        SECGetSystemtime(psec,&nmdtkd.st);

        CCSendNotify(psec->pci, DTN_WMKEYDOWN, &nmdtkd.nmhdr);

        if (psec->st.wYear   != nmdtkd.st.wYear   ||
            psec->st.wMonth  != nmdtkd.st.wMonth  ||
            psec->st.wDay    != nmdtkd.st.wDay    ||
            psec->st.wHour   != nmdtkd.st.wHour   ||
            psec->st.wMinute != nmdtkd.st.wMinute ||
            psec->st.wSecond != nmdtkd.st.wSecond) // skip wDayOfWeek and wMilliseconds
        {
            psec->st = nmdtkd.st;
            SECInvalidate(psec, SE_APP);
            return(TRUE);
        }
    }
    else
    {
        MSG msg;
        switch (wParam)
        {
        case VK_DOWN:
        case VK_SUBTRACT:
            delta = -1;
            // fall through...
        case VK_UP:
        case VK_ADD:
            PeekMessage(&msg, NULL, WM_CHAR, WM_CHAR, PM_REMOVE);  // eat this message
            SECResetSubeditEdit(pdp);
            return(SECIncrementSubedit(psec, delta));
            break;

        case VK_HOME:
        case VK_END:
            if (psec->iseCur >= 0)
            {
                LPSUBEDIT psubed;
                int valT;
                
                SECResetSubeditEdit(pdp);

                psubed = &psec->pse[psec->iseCur];
                valT = *psubed->pval;
                *psubed->pval = (wParam == VK_HOME ? psubed->min : psubed->max);
                delta = *psubed->pval - valT;
                if (delta != 0)
                {
                    SECInvalidate(psec, psubed->id);
                    return(TRUE);
                }
            }
            break;
        }
    }
    
    return(FALSE);
}

// returns TRUE if a value has changed, FALSE otherwise
// This function performs a DPNotifyDateChange() if applicable.
BOOL SECHandleChar(DATEPICK *pdp, TCHAR ch)
{
    LPSUBEDIT psubed;
    UINT uCurDigit;             // current digit hit
    UINT uCurSubValue;          // current displayed subvalue in edit field
    UINT uCurValue;             // current value of the subedit
    LPSUBEDITCONTROL psec = &pdp->sec;

    // NOTE: In almost all cases, uCurSubValue will be the same as uCurValue
    // since most fields don't have shortened displays.  However, for years
    // we can display two digits of a 4 digit number, which makes for
    // complications.
    
    if (psec->iseCur < 0)
        return(FALSE);
    
    psubed = &psec->pse[psec->iseCur];

    if (psubed->cchMax == 0)
        return(FALSE);
    
    if (ch == psec->cDelimeter || StrChr(psec->szDelimeters, ch))
    {
        SECResetSubeditEdit(pdp);
        SECIncrFocus(pdp, 1);
        return(FALSE);
    }

    // allow 'a' and 'p' to set the AM/PM fields.  we need to do some
    // funky stuff to get this to work right, so here it is.
    else if (psubed->id == SE_MARK)
    {
        if ((ch == TEXT('p') || ch == TEXT('P')) && (*psubed->pval < 12))
        {
            int valNew = *psubed->pval+12;
            
            ch = (valNew) % 10 + TEXT('0');
            psubed->valEdit = (valNew) / 10;
            psubed->cchEdit = 1;
        }
        else if ((ch == TEXT('a') || ch == TEXT('A')) && (*psubed->pval >= 12))
        {
            int valNew = *psubed->pval-12;
            ch = (valNew) % 10 + TEXT('0');
            psubed->valEdit = (valNew) / 10;
            psubed->cchEdit = 1;
        }
        else
        {
            return(FALSE);
        }
    }
    else if (ch < TEXT('0') || ch > TEXT('9'))
    {
        MessageBeep(MB_ICONHAND);
        return(FALSE);
    }
    else if (psubed->id == SE_ERA)
    {
        // I don't know what to do with this field, so bail out
        return(FALSE);
    }

    uCurDigit = ch - TEXT('0');
    if (psubed->cchEdit)
        uCurSubValue = psubed->valEdit * 10 + uCurDigit;
    else
        uCurSubValue = uCurDigit;

    uCurValue = SECAdjustByType(pdp, psubed, uCurSubValue);

    // Allow bogus values for years since you might need to type
    // in a bogus value on the way to a valid four-digit value.

    if (uCurValue > psubed->max && !SE_YEARLIKE(psubed->id))
    {
        // the number has exceeded the max, so no point in continuing
        psubed->cchEdit = 0;

        // If we're going to exceed the max, then reset the edit
        // and make this the first number instead of beeping

        uCurValue    = uCurValue - uCurSubValue + uCurDigit;
        uCurSubValue = uCurDigit;
    }

    // Allow 0 to be valEdit for subedits, even though it may be
    // illegal for that field (e.g., month).
    // This lets people type "09" and get the "expected" result.

    SECInvalidate(psec, psubed->id);

    psubed->valEdit = uCurSubValue;
    psubed->cchEdit++;
    if (psubed->cchEdit == psubed->cchMax)
        psubed->cchEdit = 0;

    if (psubed->cchEdit == 0)
    {
        // SECSetSubeditValue will do the validation
        SECSetSubeditValue(pdp, psubed, uCurSubValue, TRUE);
        return(TRUE);
    }

    if(psubed->valEdit != *psubed->pval)
        InvalidateRect(pdp->ci.hwnd, NULL, TRUE);
    return(FALSE);
}

// SECFormatSubed returns pointer to correct string
LPTSTR SECFormatSubed(LPSUBEDITCONTROL psec, LPSUBEDIT psubed, LPTSTR szTmp, UINT cch)
{
    LPTSTR sz;

    if (psubed->id == SE_STATIC)
    {
        sz = (LPTSTR)psubed->pv;
    }
    else
    {
        sz = szTmp;
        SEGetTimeDateFormat(psubed, psec, szTmp, cch);
    }

    return sz;
}

//  Returns TRUE if this subedit displays as digits (rather than text).

BOOL SECIsNumeric(LPSUBEDIT psubed)
{
    switch (psubed->id)
    {
    case SE_ERA:        return FALSE;           // g never
    case SE_YEAR:       return TRUE;            // yyyy always digits
    case SE_YEARALT:    return TRUE;            // yy always digits
    case SE_MONTH:      return lstrlen(psubed->pv) <= 2; // MM yes, but not MMM
    case SE_MONTHALT:   return lstrlen(psubed->pv) <= 4; // ddMM yes, but not ddMMM
    case SE_DAY:        return TRUE;            // dd always digits
    case SE_MARK:       return FALSE;           // tt never
    case SE_HOUR:       return TRUE;            // hh always digits
    case SE_MINUTE:     return TRUE;            // mm always digits
    case SE_SECOND:     return TRUE;            // ss always digits
    case SE_STATIC:     return FALSE;           // static text
    case SE_APP:        return FALSE;           // app's job to format this
    }
    return FALSE;
}

// SECDrawSubedits draws subedits and updates their bounding rectangles
void SECDrawSubedits(HDC hdc, LPSUBEDITCONTROL psec, BOOL fFocus, BOOL fEnabled)
{
    HGDIOBJ hfontOrig;
    int i, iseCur;
    LPTSTR sz;
    TCHAR szTmp[DTP_FORMATLENGTH];
    LPSUBEDIT psubed;

    hfontOrig = SelectObject(hdc, (HGDIOBJ)psec->hfont);

    // Do this cuz the xScroll stuff can send text into visible area that it shouldn't be in
    IntersectClipRect(hdc, psec->rc.left, psec->rc.top, psec->rc.right, psec->rc.bottom);

    SetBkColor(hdc, g_clrHighlight);

    iseCur = psec->iseCur;
    if (!fFocus)
        iseCur = SUBEDIT_NONE;

    for (i = 0, psubed = psec->pse; i < psec->cse; i++, psubed++)
    {
        RECT rc = psubed->rc;
        if (psec->xScroll)
            OffsetRect(&rc, -psec->xScroll, 0);

        if (!fEnabled)
        {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, g_clrGrayText);
        }
        else if (iseCur == i)
        {
            SetBkMode(hdc, OPAQUE);
            SetTextColor(hdc, g_clrHighlightText);
        }
        else
        {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, g_clrWindowText);
        }

        //HACK
        //if subedit control is being edited then we display the 
        //value in psubed->valEdit because it is not being updated 
        //until psubed->cchMax is reached or SECSave/ResetSubeditEdit is
        //called
        if(i == psec->iseCur && psubed->cchEdit != 0)
        {
            //
            //  If the field is numeric, then display it raw including the
            //  leading zero.  People really want to see that leading zero,
            //  so give the public what it wants.  (And even if they didn't,
            //  we need this special case anyway because the value might not
            //  yet be a valid value because the user is still typing it.
            //  This is particular true for SE_YEARLIKE fields.)
            //
            if (SECIsNumeric(psubed))
            {
                TCHAR szFormat[10];
                wsprintf(szFormat, TEXT("%%0%dd"), psubed->cchEdit);
                wsprintf(szTmp, szFormat, psubed->valEdit);
                sz = szTmp;
            }
            else
            {
                // The day-of-month might not be valid for the temporary month
                // or year in psubed->valEdit, so force the day-of-month to 1
                // so the month will always come out okay.
                //
                // This is tricky, because if the item being edited is the
                // day-of-month itself, we want to display valEdit, not 1!
                // So we force it to 1, then slam in the valEdit, then do
                // our SECFormatSubed, then restore the original values.
                //
                UINT uTmp = *psubed->pval; //save the original value
                WORD wOldDay = psec->st.wDay;
                psec->st.wDay = 1;
                // Don't change to zero in case user is typing a leading zero
                // into an alphabetic field.  (Stranger things have happened.)
                if (psubed->valEdit)
                    *psubed->pval = (WORD) psubed->valEdit;
                sz = SECFormatSubed(psec, psubed, szTmp, ARRAYSIZE(szTmp));
                psec->st.wDay = wOldDay;
                *psubed->pval = (WORD) uTmp; //restore the original value
            }
        }
        else
            sz = SECFormatSubed(psec, psubed, szTmp, ARRAYSIZE(szTmp));

        DrawText(hdc, sz, -1, &rc,
                 psubed->flDrawText | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
    }

    // we know no clip region was selected before this function
    SelectClipRgn(hdc, NULL);

    SelectObject(hdc, hfontOrig);
}

// DON'T need to worry about xScroll here because pt is offset
int SECSubeditFromPt(LPSUBEDITCONTROL psec, POINT pt)
{
    int isubed;

    for (isubed = psec->cse - 1; isubed >= 0; isubed--)
    {
        if (!psec->pse[isubed].fReadOnly &&
            pt.x >= psec->pse[isubed].rc.left)
        {
            break;
        }
    }

    return(isubed);
}

void SECGetSystemtime(LPSUBEDITCONTROL psec, LPSYSTEMTIME pst)
{
    *pst = psec->st;

    // we don't keep doy up to date, set it now (0==sun, 6==sat)
    pst->wDayOfWeek = (DowFromDate(pst)+1) % 7;  // this returns 0==sun
}

BOOL SECSetSystemtime(DATEPICK *pdp, LPSYSTEMTIME pst)
{
    pdp->sec.st = *pst;

    return TRUE; // assume something changed
}

// SECEdit: Start a free-format edit return result in szOutput.
BOOL SECEdit(DATEPICK *pdp, LPTSTR szOutput, int cchOutput)
{
    HWND      hwndEdit;
    TCHAR     szBuf[DTP_FORMATLENGTH];
    LPTSTR    pszBuf;
    int       cchBuf;
    int       i;
    int       isePrev;
    LPSUBEDIT pse;
    BOOL      fRet = FALSE;
    LPSUBEDITCONTROL psec = &pdp->sec;

    // Build the string that we hand to the app.
    // For the duration of the string build, set the current subedit
    // to SUBEDIT_ALL so that
    //  1. partial edits are applied before building the string, and
    //  2. SE_YEARALT can format appropriately.

    isePrev = psec->iseCur;
    SECSetCurSubed(pdp, SUBEDIT_ALL);
    pszBuf = szBuf;
    cchBuf = ARRAYSIZE(szBuf);

    
    //
    // Need to mirror the format since the Edit control will take
    // of the origianl format with RTL mirroring. 
    //
    if (psec->fMirrorSEC)
        pse = (psec->pse + (psec->cse - 1));
    else
        pse = psec->pse;

    for (i = 0 ; i < psec->cse ; i++)
    {
        int nTmp;

        if (pse->id == SE_STATIC)
        {
            lstrcpyn(pszBuf, pse->pv, cchBuf);
        }
        else
        {
            SEGetTimeDateFormat(pse, psec, pszBuf, cchBuf);
        }

        nTmp = lstrlen(pszBuf);

        cchBuf -= nTmp;
        pszBuf += nTmp;

        //
        // If this control is mirrored, then read contents backward.
        //
        if (psec->fMirrorSEC)
            pse--;
        else
            pse++;
    }
    SECSetCurSubed(pdp, isePrev);

    hwndEdit = CreateWindowEx(0, TEXT("EDIT"), szBuf, WS_CHILD | ES_AUTOHSCROLL,
            psec->rc.left + 2, psec->rc.top + 2,
            psec->rc.right - psec->rc.left,
            psec->rc.bottom - psec->rc.top,
            psec->pci->hwnd, NULL, HINST_THISDLL, NULL);

    if (hwndEdit)
    {
        RECT rcEdit = psec->rc;

        MapWindowRect(psec->pci->hwnd, NULL, &rcEdit); // ClientToScreen
        pdp->fFreeEditing = TRUE;
        InvalidateRect(psec->pci->hwnd, NULL, TRUE);

        Edit_LimitText(hwndEdit, ARRAYSIZE(szBuf) - 1);
        FORWARD_WM_SETFONT(hwndEdit, psec->hfont, FALSE, SendMessage);
        SetFocus(hwndEdit);
        RescrollEditWindow(hwndEdit);
        ShowWindow(hwndEdit, SW_SHOWNORMAL);

        //
        //  The basic idea:
        //
        //      Process messages until we receive a cancel message,
        //      or an accept message, or some implicit accept-like
        //      thing happens (namely, a sent WM_KILLFOCUS).
        //
        //      If the accept or cancel was implicit, then leave the
        //      cancelling message in the queue for somebody else
        //      to process.  Otherwise, if the accept/cancel was
        //      explicit, eat the message so nobody else gets
        //      confused by it.
        //
        for (;;)
        {
            MSG msg;
            BOOL fPeek;

            fPeek = PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

            // That PeekMessage may have dispatched a sent WM_KILLFOCUS,
            // in which case the change is considered to have been Accepted.
            // Leave the message we peeked in the queue because the accept
            // was implicit.
            if (GetFocus() != hwndEdit)
            {
                DebugMsg(TF_MONTHCAL, TEXT("SECEdit accept (killfocus)"));
                fRet = TRUE;
                break;

            }

            if (fPeek) {

                //
                //  Messages that cause us to cancel implicitly.
                //  These messages stay in the queue.
                //

                if (msg.message == WM_SYSCOMMAND  ||
                    msg.message == WM_SYSCHAR     ||
                    msg.message == WM_SYSDEADCHAR ||
                    msg.message == WM_DEADCHAR    ||
                    msg.message == WM_SYSKEYDOWN  ||
                    msg.message == WM_QUIT) {
                    DebugMsg(TF_MONTHCAL, TEXT("SECEdit got a message to terminate (%d)"), msg.message);
                    fRet = FALSE;
                    break;
                }

                //
                //  Messages that cause us to accept implicitly.
                //  These messages stay in the queue.
                //
                if ((msg.message == WM_LBUTTONDOWN   ||
                     msg.message == WM_NCLBUTTONDOWN ||
                     msg.message == WM_RBUTTONDOWN   ||
                     msg.message == WM_NCRBUTTONDOWN ||
                     msg.message == WM_LBUTTONDBLCLK) &&
                     !PtInRect(&rcEdit, msg.pt))
                {
                    DebugMsg(TF_MONTHCAL, TEXT("SECEdit got a message to accept (%d)"), msg.message);
                    fRet = TRUE;
                    break;
                }


                // We are now committed to eating or processing the message

                GetMessage(&msg, NULL, 0, 0);

                //
                //  Messages that cause us to cancel explicitly.
                //
                if (msg.message == WM_KEYDOWN && msg.wParam  == VK_ESCAPE)
                {
                    DebugMsg(TF_MONTHCAL, TEXT("SECEdit explicit cancel (%d)"), msg.message);
                    fRet = FALSE;
                    break;

                }

                //
                //  Messages that cause us to accept explicitly.
                //
                if (msg.message == WM_KEYDOWN && msg.wParam  == VK_RETURN)
                {
                    DebugMsg(TF_MONTHCAL, TEXT("SECEdit explicit accept (%d)"), msg.message);
                    fRet = TRUE;
                    break;
                }

                //
                //  All other messages just get dispatched.
                //
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } else {
                WaitMessage();
            }
        } // for (;;)

        if (fRet)
        {
            Edit_GetText(hwndEdit, szOutput, cchOutput);
        }
        DestroyWindow(hwndEdit);
        pdp->fFreeEditing = FALSE;
        InvalidateRect(psec->pci->hwnd, NULL, TRUE);
    }

    return(fRet);
}

//
// returns true if months were scrolled, false otherwise
//
BOOL FScrollIntoView(MONTHCAL *pmc)
{
    int nDelta = 0;
    SYSTEMTIME stEnd;

    if (MonthCal_IsMultiSelect(pmc))
        CopyDate(pmc->stEndSel, stEnd);
    else
        CopyDate(pmc->st, stEnd);
    
    //
    // If the month/yr for the new date is not in view, bring it
    // into view
    //
    if ((stEnd.wYear < pmc->stMonthFirst.wYear) ||
        ((stEnd.wYear == pmc->stMonthFirst.wYear) && (stEnd.wMonth < pmc->stMonthFirst.wMonth)))
    {
        nDelta = - (pmc->stMonthFirst.wYear - (int)stEnd.wYear) * 12 - (pmc->stMonthFirst.wMonth - (int)stEnd.wMonth);
    }
    else if ((pmc->st.wYear > pmc->stMonthLast.wYear) ||
        ((pmc->st.wYear == pmc->stMonthLast.wYear) && (pmc->st.wMonth > pmc->stMonthLast.wMonth)))
    {
        nDelta = ((int)pmc->st.wYear - pmc->stMonthLast.wYear) * 12 + ((int)pmc->st.wMonth - pmc->stMonthLast.wMonth);
    }
    
    if (nDelta)
        return FIncrStartMonth(pmc, nDelta, TRUE /* dont change day */);
    else
        return FALSE;
}

//
//  Validates the isubed to make sure we aren't setting it to something
//  bogus.  If necessary, we pick a field at random.
//
void SECSafeSetCurSubed(DATEPICK *pdp, int ise)
{
    if (ise >= pdp->sec.cse ||
        (ise >= 0 && pdp->sec.pse[ise].fReadOnly))
    {
        SECSetCurSubed(pdp, SUBEDIT_NONE);
        SECIncrFocus(pdp, 1);
    }
    else
        SECSetCurSubed(pdp, ise);
}

LRESULT DTM_OnSetFormat(DATEPICK *pdp, LPCTSTR szFormat)
{

    // remember the field that has focus so we can restore it later
    //
    int iseCur = pdp->sec.iseCur;

    if (!szFormat || !*szFormat)
    {
        pdp->fLocale = TRUE;
        DPHandleLocaleChange(pdp);
    }
    else
    {
        pdp->fLocale = FALSE;
        SECParseFormat(pdp, &pdp->sec, szFormat);
    }

    // restore focus. it might be cool to do extra validation
    // to see if iseCur is the same type that it used to be,
    // maybe even validating that cse is constant. the case we're
    // really trying to fix is changing "1st" to "2nd" to "3rd",
    // so only a text portion is really changing...
    //
    SECSafeSetCurSubed(pdp, iseCur);

    return((LRESULT)TRUE);
}
    
//
// DATEPICKER stuff
//

LRESULT CALLBACK DatePickWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DATEPICK *pdp;
    NMHDR    nmhdr;
    LRESULT  lres = 0;

    if (message == WM_NCCREATE)
        return(DPNcCreateHandler(hwnd));
    
    pdp = DatePick_GetPtr(hwnd);
    if (pdp == NULL)
        return(DefWindowProc(hwnd, message, wParam, lParam));

    // Dispatch the various messages we can receive
    switch (message)
    {
    case WM_CREATE:
        lres = DPCreateHandler(pdp, hwnd, (LPCREATESTRUCT)lParam);
        break;

    case WM_ERASEBKGND:
        if (!pdp->fEnabled) {
            RECT rc;
            HDC hdc = (HDC)wParam;

            GetClipBox(hdc, &rc);
            FillRectClr(hdc, &rc, g_clrBtnFace);
            
        } else
            goto DoDefault;
        break;
        
    case WM_PRINTCLIENT:
    case WM_PAINT:

    {
        PAINTSTRUCT ps;
        HDC hdc;

        hdc = (HDC)wParam;

        if (hdc) {
            DPPaint(pdp, hdc);
        } else {

            hwnd = pdp->ci.hwnd;
            hdc = BeginPaint(hwnd, &ps);
            DPPaint(pdp, hdc);
            EndPaint(hwnd, &ps);
        }
        break;
    }

    case WM_LBUTTONDOWN:
        DPLButtonDown(pdp, wParam, lParam);
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case MCN_SELCHANGE:
        case MCN_SELECT:
        {
            LPNMSELECT pnms = (LPNMSELECT)lParam;

            DebugMsg(TF_MONTHCAL,TEXT("MonthCal notified DateTimePick of SELECT"));
            if (!DPSetDate(pdp, &pnms->stSelStart, TRUE))
            {
                DebugMsg(DM_WARNING,TEXT("MonthCal cannot set selected date!"));
                MessageBeep(MB_ICONHAND);
            }
            pdp->fShow = (((LPNMHDR)lParam)->code == MCN_SELCHANGE);
            break;
        }

        case UDN_DELTAPOS:
            if ((int)wParam == DATEPICK_UPDOWN)
            {
                LPNM_UPDOWN pnmdp = (LPNM_UPDOWN)lParam;

                if (!pdp->fFocus)
                    SetFocus(pdp->ci.hwnd);

                SECResetSubeditEdit(pdp);

                if (SECIncrementSubedit(&pdp->sec, -pnmdp->iDelta))
                    DPNotifyDateChange(pdp);
            }
            break;
        } // WM_NOTIFY switch
        break;

    case WM_GETFONT:
        lres = (LRESULT)pdp->sec.hfont;
        break;

    case WM_SETFONT:
        DPHandleSetFont(pdp, (HFONT)wParam, (BOOL)LOWORD(lParam));
        break;

    case WM_DESTROY:
        DPDestroyHandler(hwnd, pdp, wParam, lParam);
        break;
        
    case WM_KILLFOCUS:
    case WM_SETFOCUS:
    {
        BOOL fGotFocus = (message == WM_SETFOCUS);
        if (BOOLIFY(fGotFocus) != BOOLIFY(pdp->fFocus))
        {
            pdp->fFocus = (WORD) fGotFocus;
            if (pdp->sec.iseCur >= 0)
            {
                InvalidateScrollRect(pdp->ci.hwnd, &pdp->sec.pse[pdp->sec.iseCur].rc, pdp->sec.xScroll);
            }
            else if (DatePick_ShowCheck(pdp))
            {
                pdp->fCheckFocus = (WORD) fGotFocus;
                InvalidateRect(pdp->ci.hwnd, &pdp->rcCheck, TRUE);
            }
            else if (fGotFocus) // nothing has focus, bring it to something
            {
                SECIncrFocus(pdp, 1);
            }
            
            CCSendNotify(&pdp->ci, (fGotFocus ? NM_SETFOCUS : NM_KILLFOCUS), &nmhdr);
        }

        if (fGotFocus)
        {
            // Revalidate iseLastActive because the app might've changed
            // the format while we were nonfocus
            SECSafeSetCurSubed(pdp, pdp->iseLastActive);
        }
        else
        {
            pdp->iseLastActive = pdp->sec.iseCur;
            SECSetCurSubed(pdp, SUBEDIT_NONE);
        }

        break;
    }

    case WM_ENABLE:
    {
        BOOL fEnabled = wParam ? TRUE:FALSE;
        if (BOOLIFY(pdp->fEnabled) != fEnabled)
        {
            pdp->fEnabled = (WORD) fEnabled;
            if (pdp->hwndUD)
                EnableWindow(pdp->hwndUD, fEnabled);
            InvalidateRect(pdp->ci.hwnd, NULL, TRUE);
        }
        break;
    }

    case DTMP_WINDOWPOSCHANGED:
    case WM_SIZE:
    {
        RECT rc;

        if (message == DTMP_WINDOWPOSCHANGED)
        {
            GetClientRect(pdp->ci.hwnd, &rc);
        }
        else
        {
            rc.left   = 0;
            rc.top    = 0;
            rc.right  = GET_X_LPARAM(lParam);
            rc.bottom = GET_Y_LPARAM(lParam);
        }

        DPRecomputeSizing(pdp, &rc);

        InvalidateRect(pdp->ci.hwnd, NULL, TRUE);
        UpdateWindow(pdp->ci.hwnd);
        break;
    }

    case WM_GETDLGCODE:
        lres = DLGC_WANTARROWS | DLGC_WANTCHARS;
        break;

    case WM_KEYDOWN:        
        if (pdp->fShow)
        {
            SendMessage(pdp->hwndMC, WM_KEYDOWN, wParam, lParam);
            return 0;
        }
        else
        {
            lres = DPHandleKeydown(pdp, wParam, lParam);
        }
        break;

    case WM_KEYUP:
        if (pdp->fShow)
            SendMessage(pdp->hwndMC, WM_KEYUP, wParam, lParam);
        break;
        
    case WM_SYSKEYDOWN:
        if (wParam == VK_DOWN && !pdp->fUseUpDown)
        {
            DPLBD_MonthCal(pdp, FALSE);
        }
        else
            goto DoDefault;
        break;

    case WM_CHAR:
        lres = DPHandleChar(pdp, wParam, lParam);
        break;

    case WM_SYSCOLORCHANGE:
        InitGlobalColors();
        // Don't need to propagate to pdp->hwndMC because it is its own
        // top-level window.
        break;

    case WM_WININICHANGE:
        if (lParam == 0 ||
#ifdef UNICODE_WIN9x
            !lstrcmpiA((LPSTR)lParam, "Intl")
#else
            !lstrcmpi((LPTSTR)lParam, TEXT("Intl"))
#endif
           )
        {
            DPHandleLocaleChange(pdp);
        }
        // Don't need to propagate to pdp->hwndMC because it is its own
        // top-level window.
        break;

    case WM_NOTIFYFORMAT:
        return CIHandleNotifyFormat(&pdp->ci, lParam);
        break;

    // Cannot use WM_SETTEXT to change the text of a DTP
    case WM_SETTEXT:
        return -1;

    case WM_GETTEXT:
        if (!lParam || !wParam) {
            // previously this just failed and returned 0
            // in bogus input.  should be safe to convert to
            // gettextlength
            message = WM_GETTEXTLENGTH;
        } else 
            (*(LPTSTR)lParam) = 0;
        
        // fall through
        
    case WM_GETTEXTLENGTH:
    {
        TCHAR     szTmp[DTP_FORMATLENGTH];
        LPSUBEDIT psubed;
        int       i;
#ifdef UNICODE_WIN9x
        char *pszText = (char *)lParam;
#else
        TCHAR *pszText = (TCHAR *)lParam;
#endif
        UINT      nTextLen = 0;

        for (i = 0, psubed = pdp->sec.pse; i < pdp->sec.cse; i++, psubed++)
        {
            LPTSTR sz;
            UINT nLen;

            sz = SECFormatSubed(&pdp->sec, psubed, szTmp, ARRAYSIZE(szTmp));
            nLen = lstrlen(sz);

            if (message == WM_GETTEXT) {
                if (nTextLen + nLen >= wParam)
                    break;

#ifdef UNICODE_WIN9x
                // safe to pass wparam because we calculated above that it was ok
                ConvertWToAN(CP_ACP, pszText, wParam, sz, -1);
#else
                lstrcpy(pszText, sz);
#endif
                pszText  += nLen;
            }
            
            nTextLen += nLen;
        }
        lres = nTextLen;
    }
    break;

    case WM_STYLECHANGING:
        lres = DPOnStyleChanging(pdp, (UINT) wParam, (LPSTYLESTRUCT)lParam);
        break;

    case WM_STYLECHANGED:
        lres = DPOnStyleChanged(pdp, (UINT) wParam, (LPSTYLESTRUCT)lParam);
        break;

    case WM_CONTEXTMENU:
        if (pdp->hwndMC)
            lres = SendMessage(pdp->hwndMC, message, wParam, lParam);
        else
            goto DoDefault;
        break;


    //
    // DATETIMEPICK specific messages
    //

    // DTM_GETSYSTEMTIME wParam=void lParam=LPSYSTEMTIME
    //   returns GDT_NONE if no date selected (DTS_SHOWNONE only)
    //   returns GDT_VALID and modifies *lParam to be the selected date
    case DTM_GETSYSTEMTIME:
        if (!pdp->fCheck)
        {
            lres = GDT_NONE;
        }
        else
        {
            // If there is an edit pending, save it so the app sees
            // the absolute latest values.  This is important for app
            // compat, because IE4 wasn't Y2K compliant and people got
            // away with typing just two digits of the year and hitting
            // ENTER.  The "Find Files" dialog would then ask us for the
            // year, and in the Y2K case, we are still waiting for the
            // other two digits (for a four-digit year) and return the
            // wrong year.
            SECSaveSubeditEdit(pdp);
            SECGetSystemtime(&pdp->sec, (SYSTEMTIME *)lParam);
            lres = GDT_VALID;
        }
        break;

    // DTM_SETSYSTEMTIME wParam=GDT_flag lParam=LPSYSTEMTIME
    //   if wParam==GDT_NONE, sets datepick to None (DTS_SHOWNONE only)
    //   if wParam==GDT_VALID, sets datepick to *lParam
    //   returns TRUE on success, FALSE on error (such as bad params)
    case DTM_SETSYSTEMTIME:
    {
        LPSYSTEMTIME pst = ((LPSYSTEMTIME)lParam);

        if ((wParam != GDT_NONE  && wParam != GDT_VALID)      ||
            (wParam == GDT_NONE  && !DatePick_ShowCheck(pdp)) ||
            (wParam == GDT_VALID && !IsValidSystemtime(pst)))
        {
            break;
        }

        // reset subed in place edit
        SECResetSubeditEdit(pdp);

        pdp->fNoNotify = TRUE;
        if (DatePick_ShowCheck(pdp))
        {
            if ((wParam == GDT_NONE) || (pdp->fCheck))
            {
                // let checkbox have focus
                SECSetCurSubed(pdp, SUBEDIT_NONE);
                pdp->fCheckFocus = 1;
            }

            pdp->fCheck = (wParam == GDT_NONE ? 0 : 1);            
            InvalidateRect(pdp->ci.hwnd, NULL, TRUE);
        }
        if (wParam == GDT_VALID)
        {
            pdp->fNoNotify = TRUE;
            DPSetDate(pdp, pst, FALSE);
            pdp->fNoNotify = FALSE;
        }
        lres = TRUE;
        pdp->fNoNotify = FALSE;

        break;
    }

    // DTM_GETRANGE wParam=void lParam=LPSYSTEMTIME[2]
    //   modifies *lParam to be the minimum ALLOWABLE systemtime (or 0 if no minimum)
    //   modifies *(lParam+1) to be the maximum ALLOWABLE systemtime (or 0 if no maximum)
    //   returns GDTR_MIN|GDTR_MAX if there is a minimum|maximum limit
    case DTM_GETRANGE:
    {
        LPSYSTEMTIME pst = (LPSYSTEMTIME)lParam;

        ZeroMemory(pst, 2 * sizeof(SYSTEMTIME));
        lres = pdp->gdtr;
        if (lres & GDTR_MIN)
            pst[0] = pdp->stMin;
        if (lres & GDTR_MAX)
            pst[1] = pdp->stMax;
        break;
    }

    // DTM_SETRANGE wParam=GDR_flags lParam=LPSYSTEMTIME[2]
    //   if GDTR_MIN, sets the minimum ALLOWABLE systemtime to *lParam, otherwise removes minimum
    //   if GDTR_MAX, sets the maximum ALLOWABLE systemtime to *(lParam+1), otherwise removes maximum
    //   returns TRUE on success, FALSE on error (such as invalid parameters)
    case DTM_SETRANGE:
    {
        LPSYSTEMTIME pst = (LPSYSTEMTIME)lParam;
        const SYSTEMTIME *pstMin = (wParam & GDTR_MIN) ? pst+0 : &c_stEpoch;
        const SYSTEMTIME *pstMax = (wParam & GDTR_MAX) ? pst+1 : &c_stArmageddon;

        if (!IsValidDate(pstMin) || !IsValidDate(pstMax))
        {
            break;
        }

        // Save the flags so we can tell the app if it asks.
        // We personally don't care.
        pdp->gdtr = (UINT)wParam & (GDTR_MIN | GDTR_MAX);

        if (CmpDate(&pdp->stMin, &pdp->stMax) <= 0)
        {
            pdp->stMin = *pstMin;
            pdp->stMax = *pstMax;
        }
        else
        {
            pdp->stMin = *pstMax;
            pdp->stMax = *pstMin;
        }

        // we might now have an invalid date, if so, try to set the current
        // date and munge it to a max or min value if out of range.
        pdp->fNoNotify = TRUE;
        DPSetDate(pdp, &pdp->sec.st, TRUE);
        pdp->fNoNotify = FALSE;
        lres = TRUE;
        break;
    }

#ifdef UNICODE
    // DTM_SETFORMAT wParam=void lParam=LPCTSTR
    //   Sets the formatting string to a copy of lParam.
    case DTM_SETFORMATA:
    {
        LPCSTR pszFormat = (LPCSTR)lParam;
        LPWSTR pwszFormat = NULL;

        if (pszFormat && *pszFormat)
        {
            pwszFormat = ProduceWFromA(pdp->ci.uiCodePage, pszFormat);
        }

        lres = DTM_OnSetFormat(pdp, pwszFormat);

        if (pwszFormat)
        {
            FreeProducedString(pwszFormat);
        }
        break;
    }
#endif

    // DTM_SETFORMAT wParam=void lParam=LPCTSTR
    //   Sets the formatting string to a copy of lParam.
    case DTM_SETFORMAT:
    {
        lres = DTM_OnSetFormat(pdp, (LPCTSTR)lParam);
        break;
    }
        
    case DTM_SETMCCOLOR:
        if (wParam >= 0 && wParam < MCSC_COLORCOUNT) 
        {
            COLORREF clr = pdp->clr[wParam];
            pdp->clr[wParam] = (COLORREF)lParam;
            if (pdp->hwndMC)
                SendMessage(pdp->hwndMC, MCM_SETCOLOR, wParam, lParam);
            return clr;
        }
        return -1;
        
    case DTM_GETMCCOLOR:
        if (wParam >= 0 && wParam < MCSC_COLORCOUNT) 
            return pdp->clr[wParam];
        return -1;
        
    case DTM_GETMONTHCAL:
        return (LRESULT)(UINT_PTR)pdp->hwndMC;

    // wParam -- HFONT, LOWORD(lParam) -- fRedraw
    case DTM_SETMCFONT:
        pdp->hfontMC = (HFONT)wParam;
        if (pdp->hwndMC)
            SendMessage(pdp->hwndMC, WM_SETFONT, wParam, lParam);        
        break;

    // returns the font
    case DTM_GETMCFONT:
        return (LRESULT)pdp->hfontMC;
        break;

    default:
        if (CCWndProc(&pdp->ci, message, wParam, lParam, &lres))
            return lres;

DoDefault:
        lres = DefWindowProc(hwnd, message, wParam, lParam);
        break;
    } /* switch (message) */

    return(lres);
}

LRESULT DPNcCreateHandler(HWND hwnd)
{
    DATEPICK *pdp;

    // Sink the datepick -- we may only want to do this if WS_BORDER is set
    SetWindowBits(hwnd, GWL_EXSTYLE, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);

    // Allocate storage for the dtpick structure
    pdp = (DATEPICK *)NearAlloc(sizeof(DATEPICK));
    if (pdp)
        DatePick_SetPtr(hwnd, pdp);

    return((LRESULT)pdp);
}

void DPDestroyHandler(HWND hwnd, DATEPICK *pdp, WPARAM wParam, LPARAM lParam)
{
    if (pdp)
    {
        SECDestroy(&pdp->sec);
        MCFreeCalendarInfo(&pdp->sec.ct);
        GlobalFreePtr(pdp);
    }

    DatePick_SetPtr(hwnd, NULL);
}

// set any locale-dependent values
#define DTS_TIMEFORMATONLY (DTS_TIMEFORMAT & ~DTS_UPDOWN) // remove the UPDOWN bit for testing

LRESULT DPCreateHandler(DATEPICK *pdp, HWND hwnd, LPCREATESTRUCT lpcs)
{
    HFONT      hfont;
    SYSTEMTIME st;
    LCID       lcid;

    // Initialize our data.
    CIInitialize(&pdp->ci, hwnd, lpcs);

    if (pdp->ci.style & DTS_INVALIDBITS)
        return(-1);

    if (pdp->ci.style & DTS_UPDOWN)
    {
        pdp->fUseUpDown = TRUE;
        pdp->hwndUD = CreateWindow(UPDOWN_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | (pdp->ci.style & WS_DISABLED),
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwnd,
            (HMENU)DATEPICK_UPDOWN, HINST_THISDLL, NULL);
    }
    if (DatePick_ShowCheck(pdp))
    {
        pdp->sec.fNone = TRUE; // ugly: this SEC stuff should be merged back into DATEPICK
        pdp->iseLastActive = SUBEDIT_NONE;
    }

    pdp->fEnabled = !(pdp->ci.style & WS_DISABLED);
    pdp->fCheck   = TRUE; // start checked

    // Default minimum date is the epoch
    pdp->stMin = c_stEpoch;

    // Default maximum date is armageddon
    pdp->stMax = c_stArmageddon;

    pdp->gdtr = GDTR_MIN;           // We marked MIN as set in IE4, go figure

    //
    // See if the date/time picker supports this calendar. [samera]
    //
    MCGetCalendarInfo(&pdp->sec.ct);

    //
    // If the DTP is RTL mirrored and it's a Time-Only field, then
    // we need to mirror format string so that it's displayed correctly
    // on a RTL mirrored window. In case of Arabic, we need to swap the 
    // Time-Marker to the other side (visual left) so that it looks ok.
    // For the hebrew, we need to swap the field (whether it's date or time)
    // bacause unlike Arabic, it doesn't have its own digit so it reads
    // from LeftToRight. [samera]
    //
    lcid = GetUserDefaultLCID();
    pdp->sec.fMirrorSEC = pdp->sec.fSwapTimeMarker = FALSE;
    if (IS_WINDOW_RTL_MIRRORED(hwnd))
    {
        if (pdp->ci.style & DTS_TIMEFORMATONLY)
        {
            pdp->sec.fMirrorSEC = TRUE;

            if ((PRIMARYLANGID(LANGIDFROMLCID(lcid))) == LANG_ARABIC)
                pdp->sec.fSwapTimeMarker = TRUE;
        }
        else if((PRIMARYLANGID(LANGIDFROMLCID(lcid))) == LANG_HEBREW)
        {
            pdp->sec.fMirrorSEC = TRUE;
        }
    }


    // initialize SUBEDITCONTROL
    pdp->sec.pci = &pdp->ci;
    GetLocalTime(&st);
    SECSetSystemtime(pdp, &st);
    SECSetFont(&pdp->sec, NULL);
    pdp->fLocale = TRUE;
    DPHandleLocaleChange(pdp);
    MCLoadString(IDS_DELIMETERS, pdp->sec.szDelimeters, ARRAYSIZE(pdp->sec.szDelimeters));

    
    hfont = NULL;
    if (lpcs->hwndParent)
        hfont = (HFONT)SendMessage(lpcs->hwndParent, WM_GETFONT, 0, 0);
    DPHandleSetFont(pdp, hfont, FALSE);
    
    // initialize the colors
    MCInitColorArray(pdp->clr);
    return(0);
}

LRESULT DPOnStyleChanging(DATEPICK *pdp, UINT gwl, LPSTYLESTRUCT pinfo)
{
    if (gwl == GWL_STYLE)
    {
        DWORD changeFlags = pdp->ci.style ^ pinfo->styleNew;

        // Don't allow these bits to change
        changeFlags &= DTS_UPDOWN | DTS_SHOWNONE | DTS_INVALIDBITS;

        pinfo->styleNew ^= changeFlags;
    }

    return(0);
}

LRESULT DPOnStyleChanged(DATEPICK *pdp, UINT gwl, LPSTYLESTRUCT pinfo)
{
    if (gwl == GWL_STYLE)
    {
        DWORD changeFlags = pdp->ci.style ^ pinfo->styleNew;

        ASSERT(!(changeFlags & (DTS_UPDOWN|DTS_SHOWNONE)));

        pdp->ci.style = pinfo->styleNew;

        if (changeFlags & (DTS_SHORTDATEFORMAT|DTS_LONGDATEFORMAT|DTS_TIMEFORMAT|DTS_INVALIDBITS))
        {
            DPHandleLocaleChange(pdp);
        }

        if (changeFlags & (WS_BORDER | WS_CAPTION | WS_THICKFRAME)) {
            // the changing of these bits affect the size of the window
            // but not until after this message is handled
            // so post ourself a message.
            PostMessage(pdp->ci.hwnd, DTMP_WINDOWPOSCHANGED, 0, 0);
        }

    }

    return(0);
}


void DPHandleLocaleChange(DATEPICK *pdp)
{
    //
    // See if the date/time picker supports this new calendar, and refresh
    // era names as appropriate.
    //
    MCGetCalendarInfo(&pdp->sec.ct);

    if (pdp->fLocale)
    {
        TCHAR szFormat[DTP_FORMATLENGTH];

        switch (pdp->ci.style & DTS_FORMATMASK)
        {
        case DTS_TIMEFORMATONLY:
            GetLocaleInfo(pdp->sec.ct.lcid, LOCALE_STIMEFORMAT, szFormat, ARRAYSIZE(szFormat));
            break;

        case DTS_LONGDATEFORMAT:
            GetLocaleInfo(pdp->sec.ct.lcid, LOCALE_SLONGDATE, szFormat, ARRAYSIZE(szFormat));
            break;

        case DTS_SHORTDATEFORMAT:
        case DTS_SHORTDATECENTURYFORMAT:
            GetLocaleInfo(pdp->sec.ct.lcid, LOCALE_SSHORTDATE, szFormat, ARRAYSIZE(szFormat));
            break;
        }
        SECParseFormat(pdp, &pdp->sec, szFormat);
    }
}

void DPHandleSetFont(DATEPICK *pdp, HFONT hfont, BOOL fRedraw)
{
    SECSetFont(&pdp->sec, hfont);
    SECRecomputeSizing(&pdp->sec, &pdp->rc);
    pdp->ci.uiCodePage = GetCodePageForFont(hfont);

    if (fRedraw)
    {
        InvalidateRect(pdp->ci.hwnd, NULL, TRUE);
        UpdateWindow(pdp->ci.hwnd);
    }
}

void DPPaint(DATEPICK *pdp, HDC hdc)
{
    if (DatePick_ShowCheck(pdp))
    {
        if (RectVisible(hdc, &pdp->rcCheck))
        {
            RECT rc;
            UINT dfcs;
            rc = pdp->rcCheck;
            if (pdp->fCheckFocus)
                DrawFocusRect(hdc, &rc);

            InflateRect(&rc, -1 , -1);
            dfcs = DFCS_BUTTONCHECK;
            if (pdp->fCheck)
                dfcs |= DFCS_CHECKED;
            if (!pdp->fEnabled)
                dfcs |= DFCS_INACTIVE;
            DrawFrameControl(hdc, &rc, DFC_BUTTON, dfcs);
        }
    }

    if (!pdp->fFreeEditing)
        SECDrawSubedits(hdc, &pdp->sec, pdp->fFocus, pdp->fCheck ? pdp->fEnabled : FALSE);

    if (!pdp->fUseUpDown && RectVisible(hdc, &pdp->rcBtn))
        DPDrawDropdownButton(pdp, hdc, FALSE);
}

void _RecomputeMonthCalRect(DATEPICK *pdp, LPRECT prcCal, LPRECT prcCalT )
{
    RECT rcCal  = *prcCal;
    RECT rcCalT = *prcCalT;
    RECT rcWorkArea;
    MONITORINFO mi = {0};
    HMONITOR hMonitor;
    
    if (DatePick_RightAlign(pdp))
    {
        rcCal.left = rcCal.right - (rcCalT.right - rcCalT.left);
    }
    else
    {
        rcCal.right = rcCal.left + (rcCalT.right - rcCalT.left);
    }
    rcCal.bottom = rcCal.top + (rcCalT.bottom - rcCalT.top);

    // Get the information about the most appropriate monitor.
    // (This includes both the work area and the monitor size.
    hMonitor = MonitorFromRect(&rcCal, MONITOR_DEFAULTTONEAREST);
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);
    
    // we need to know where to fit this rectangle into
    if (GetWindowLong(pdp->ci.hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
    {
        // if we're topmost, our limits are the screen limits (not the working area)
        rcWorkArea = mi.rcMonitor;
    }
    else
    {
        // otherwise it's the limits of the workarea
        rcWorkArea = mi.rcWork;
    }

    // slide left if off the right side of area
    if (rcCal.right > rcWorkArea.right)
    {
        int nTmp = rcCal.right - rcWorkArea.right;
        rcCal.left  -= nTmp;
        rcCal.right -= nTmp;
    }
    
    // slide right if off the left side of area
    if (rcCal.left < rcWorkArea.left)
    {
        int nTmp = rcWorkArea.left - rcCal.left;
        rcCal.left  += nTmp;
        rcCal.right += nTmp;
    }
    
    // move to top of control if off the bottom side of area
    if (rcCal.bottom > rcWorkArea.bottom)
    {
        RECT rcT = pdp->rc;
        int nTmp = rcCal.bottom - rcCal.top;

        MapWindowRect(pdp->ci.hwnd, NULL, (LPPOINT)&rcT); // 2 ClientToScreen

        rcCal.bottom = rcT.top;
        rcCal.top    = rcCal.bottom - nTmp;
    }
    
    *prcCal = rcCal;    
}

void DPLBD_MonthCal(DATEPICK *pdp, BOOL fLButtonDown)
{
    HDC  hdc;
    HWND hwndMC;
    RECT rcT, rcCalT;
    RECT rcBtn, rcCal;
    BOOL fBtnDown;      // Is the button drawn DOWN or UP
    BOOL fBtnActive;    // Is the button still active
    SYSTEMTIME st;
    SYSTEMTIME stOld;
    DWORD dwWidth;
    
    hdc = GetDC(pdp->ci.hwnd);

    // turn datetimepick on but remove all focus -- the MonthCal will have focus
    if (!pdp->fCheck)
    {
        pdp->fCheck = TRUE;
        InvalidateRect(pdp->ci.hwnd, NULL, TRUE);
        DPNotifyDateChange(pdp);
    }
    if (pdp->fCheckFocus)
    {
        pdp->fCheckFocus = FALSE;
        InvalidateRect(pdp->ci.hwnd, &pdp->rcCheck, TRUE);
    }
    SECSetCurSubed(pdp, SUBEDIT_NONE);

    if (fLButtonDown)
        DPDrawDropdownButton(pdp, hdc, TRUE);

    rcT = pdp->rc;
    MapWindowRect(pdp->ci.hwnd, NULL, &rcT); //2 ClientToScreen

    rcBtn = pdp->rcBtn;
    MapWindowRect(pdp->ci.hwnd, NULL, &rcBtn); //ClientToScreen

    rcCal = rcT;                       // this size is only temp until
    rcCal.top    = rcCal.bottom + 1;   // we ask the monthcal how big it
    rcCal.bottom = rcCal.top + 1;      // wants to be

    hwndMC = CreateWindow(g_rgchMCName, NULL, WS_POPUP | WS_BORDER,
                    rcCal.left, rcCal.top,
                    rcCal.right - rcCal.left, rcCal.bottom - rcCal.top,
                    pdp->ci.hwnd, NULL, HINST_THISDLL, NULL);
    if (hwndMC == NULL)
    {
        // BUGBUG: we are left with the button drawn DOWN
        DebugMsg(DM_WARNING, TEXT("DPLBD_MonthCal could not create MONTHCAL"));
        return;
    }

    pdp->hwndMC = hwndMC;

    // set all the colors:
    {
        int i;
        for (i = 0; i < MCSC_COLORCOUNT; i++)
        {
            SendMessage(hwndMC, MCM_SETCOLOR, i, pdp->clr[i]);
        }
    }

    if (pdp->hfontMC)
        SendMessage(hwndMC, WM_SETFONT, (WPARAM)pdp->hfontMC, (LPARAM)FALSE);

    // set min/max dates
    // Relies on HACK! that stMin and stMax are adjacent
    MonthCal_SetRange(hwndMC, GDTR_MIN | GDTR_MAX, &pdp->stMin);

    SendMessage(hwndMC, MCM_GETMINREQRECT, 0, (LPARAM)&rcCalT);
    ASSERT(rcCalT.left == 0 && rcCalT.top == 0);
    dwWidth = (DWORD)SendMessage(hwndMC, MCM_GETMAXTODAYWIDTH, 0, 0);
    if (dwWidth > (DWORD)rcCalT.right)
        rcCalT.right = dwWidth;

    SECGetSystemtime(&pdp->sec, &st);
    SendMessage(hwndMC, MCM_SETCURSEL, 0, (LPARAM)&st);

    _RecomputeMonthCalRect(pdp, &rcCal, &rcCalT);
    MoveWindow(hwndMC, rcCal.left, rcCal.top,
        rcCal.right - rcCal.left, rcCal.bottom - rcCal.top, FALSE);

    CCSendNotify(&pdp->ci, DTN_DROPDOWN, NULL);

    //
    // HACK-- App may have resized the window during DTN_DROPDOWN,
    // so we need to get the new rcCal rect
    //
    {
        MONTHCAL *pmc = MonthCal_GetPtr(hwndMC);
        _RecomputeMonthCalRect(pdp, &rcCal, &pmc->rc);
        MoveWindow(hwndMC, rcCal.left, rcCal.top,
            rcCal.right - rcCal.left, rcCal.bottom - rcCal.top, FALSE);

#ifdef DEBUG
        if (GetAsyncKeyState(VK_CONTROL) < 0)
            (pmc)->ci.style |= MCS_MULTISELECT;
#endif
    }
    
    ShowWindow(hwndMC, SW_SHOWNA);
    
    pdp->fShow = TRUE;
    fBtnDown   = fLButtonDown;
    fBtnActive = fLButtonDown;

    stOld = pdp->sec.st;
    
    while (pdp->fShow)
    {
        MSG msg;

        pdp->fShow = (WORD) GetMessage(&msg, NULL, 0, 0);

        // Here's how button controls work as far as I can tell:
        // Until the "final button draw up", the button draws down when the
        // mouse is over it and it draws up when the mouse is not over it. This
        // entire time, the control is active.
        //
        // The "final button draw up" occurs at the first opportunity of:
        // the user releases the mouse button OR the user moves into the rect
        // of the control.  The control does it's action on a "mouse up".

        if (fBtnActive)
        {
            switch (msg.message) {
            case WM_MOUSEMOVE:
                if (PtInRect(&rcBtn, msg.pt))
                {
                    if (!fBtnDown)
                    {
                        DPDrawDropdownButton(pdp, hdc, TRUE);
                        fBtnDown = TRUE;
                    }
                }
                else
                {
                    if (fBtnDown)
                    {
                        DPDrawDropdownButton(pdp, hdc, FALSE);
                        fBtnDown = FALSE;
                    }
                    if (PtInRect(&rcCal, msg.pt))
                    {
                        fBtnActive = FALSE;
                        // let MonthCal think it got a button down
                        // IEUNIX: dumb compiler doesn't do well with comments in macros.
                        FORWARD_WM_LBUTTONDOWN(hwndMC, FALSE,
                            rcCal.left/2 + rcCal.right/2, 
                            rcCal.top/2 + rcCal.bottom/2, 
                            0, SendMessage);
                    }
                }
                continue; // the MonthCal doesn't need this message
                
            case WM_LBUTTONUP:
                if (fBtnDown)
                {
                    DPDrawDropdownButton(pdp, hdc, FALSE);
                    fBtnDown = FALSE;
                }
                fBtnActive = FALSE;
                continue; // the MonthCal doesn't need this message
            }
        } // if (fBtnActive)

        // Check for events that cause the calendar to go away

        //
        //  These events mean "I like it".  We allow Alt+Up or Enter
        //  to accept the changes.  (Alt+Up for compat with combo boxes.)
        //
        if (((msg.message == WM_LBUTTONDOWN   ||
              msg.message == WM_NCLBUTTONDOWN ||
              msg.message == WM_LBUTTONDBLCLK) && !PtInRect(&rcCal, msg.pt))  ||
              msg.message == WM_SYSCOMMAND    ||
              msg.message == WM_COMMAND       ||
              (msg.message == WM_SYSKEYDOWN && msg.wParam == VK_UP) ||
              (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) ||
              msg.message == WM_KILLFOCUS)
        {
            DebugMsg(TF_MONTHCAL,TEXT("DPLBD_MonthCal got a message to accept (%d)"), msg.message);
            pdp->fShow = FALSE;
            continue;
        }

        //
        //  These events mean "I don't like it".
        //
        else if (((msg.message == WM_RBUTTONDOWN   ||
                   msg.message == WM_NCRBUTTONDOWN ||
                   msg.message == WM_RBUTTONDBLCLK) && !PtInRect(&rcCal, msg.pt)) ||
                (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE))
        {
            DebugMsg(TF_MONTHCAL,TEXT("DPLBD_MonthCal got a message to cancel (%d)"), msg.message);
            pdp->fShow = FALSE;
            pdp->sec.st = stOld;
            DPNotifyDateChange(pdp);
            InvalidateRect(pdp->ci.hwnd, NULL, TRUE);
            continue;
        }

       
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    } // while(fShow)

    CCSendNotify(&pdp->ci, DTN_CLOSEUP, NULL);

    pdp->hwndMC = NULL;
    DestroyWindow(hwndMC);
    ReleaseDC(pdp->ci.hwnd, hdc);
}

void DPHandleSECEdit(DATEPICK *pdp)
{
    TCHAR szBuf[DTP_FORMATLENGTH];

    if (SECEdit(pdp, szBuf, ARRAYSIZE(szBuf)))
    {
        NMDATETIMESTRING nmdts = {0};

        nmdts.pszUserString = szBuf;
        // just in case the app doesn't parse the string
        nmdts.st      = pdp->sec.st;
        nmdts.dwFlags = (pdp->fCheck==1) ? GDT_VALID : GDT_NONE;

        CCSendNotify(&pdp->ci, DTN_USERSTRING, &nmdts.nmhdr);

        // If the app gives us an invalid date, go back to the old date
        if (nmdts.dwFlags == GDT_VALID &&
            !IsValidSystemtime(&nmdts.st))
        {
            nmdts.st = pdp->sec.st;
        }

        if (nmdts.dwFlags == GDT_NONE)
        {
            if (DatePick_ShowCheck(pdp))
            {
                pdp->fCheck      = FALSE;
                pdp->fCheckFocus = TRUE;
                SECSetCurSubed(pdp, SUBEDIT_NONE);
                InvalidateRect(pdp->ci.hwnd, NULL, TRUE);
                DPNotifyDateChange(pdp);
            }
        }
        else if (nmdts.dwFlags == GDT_VALID)
        {
            DPSetDate(pdp, &nmdts.st, FALSE);
        }
    }
}

LRESULT DPLButtonDown(DATEPICK *pdp, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    BOOL  fFocus;

    if (!pdp->fEnabled)
        return(0);

    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    // reset subed char count
    SECResetSubeditEdit(pdp);

    fFocus = pdp->fFocus;
    if (!fFocus)
        SetFocus(pdp->ci.hwnd);

    // display MONTHCAL iif we're not DTS_UPDOWN
    if (!pdp->fUseUpDown && PtInRect(&pdp->rcBtn, pt) && IsWindowVisible(pdp->ci.hwnd))
    {
        DPLBD_MonthCal(pdp, TRUE);
    }
    else if (!pdp->fCapture)
    {
        // Un/check checkbox
        if (DatePick_ShowCheck(pdp) && PtInRect(&pdp->rcCheck, pt))
        {
            pdp->fCheck      = !pdp->fCheck;
            pdp->fCheckFocus = 1;
            SECSetCurSubed(pdp, SUBEDIT_NONE);
            InvalidateRect(pdp->ci.hwnd, NULL, TRUE);
            DPNotifyDateChange(pdp);
        }
        
        // Select a subedit
        else if (pdp->fCheck)
        {
            if (DatePick_AppCanParse(pdp) && fFocus)
            {
                // First click brings focus to a subedit, second click starts editing
                DPHandleSECEdit(pdp);
            }
            else
            {
                int isubed;
                pt.x += pdp->sec.xScroll;
                isubed = SECSubeditFromPt(&pdp->sec, pt);
                if (isubed >= 0)
                {
                    SECSetCurSubed(pdp, isubed);
                    if (DatePick_ShowCheck(pdp))
                    {
                        pdp->fCheckFocus = 0;
                        InvalidateRect(pdp->ci.hwnd, &pdp->rcCheck, TRUE);
                    }
                }
            }
        }
    }
    
    return(0);
}

void DPRecomputeSizing(DATEPICK *pdp, RECT *prect)
{
    RECT rcTmp;

    if (DatePick_ShowCheck(pdp))
    {
        pdp->rcCheck.top    = prect->top    + 1;
        pdp->rcCheck.bottom = prect->bottom - 1;
        pdp->rcCheck.left   = prect->left   + 1;
        pdp->rcCheck.right  = prect->left   + (pdp->rcCheck.bottom - pdp->rcCheck.top);
        
        // occupy at most half the width of the window
        if (pdp->rcCheck.right > prect->left + (prect->right - prect->left)/2)
        {
            pdp->rcCheck.right = prect->left + (prect->right - prect->left)/2;
        }
    }
    else
    {
        pdp->rcCheck.top    = prect->top;
        pdp->rcCheck.bottom = prect->top;
        pdp->rcCheck.left   = prect->left;
        pdp->rcCheck.right  = prect->left + DPXBUFFER - 1;
    }

    pdp->rcBtn = *prect;
    pdp->rcBtn.left = pdp->rcBtn.right - GetSystemMetrics(SM_CXVSCROLL);
    if (pdp->rcBtn.left < pdp->rcCheck.right)
        pdp->rcBtn.left = pdp->rcCheck.right;
    if (pdp->hwndUD)
        MoveWindow(pdp->hwndUD, pdp->rcBtn.left, pdp->rcBtn.top, pdp->rcBtn.right - pdp->rcBtn.left + 1, pdp->rcBtn.bottom - pdp->rcBtn.top + 1, FALSE);

    rcTmp = pdp->rc;
    pdp->rc.top    = prect->top;
    pdp->rc.bottom = prect->bottom;
    pdp->rc.left   = pdp->rcCheck.right + 1;
    pdp->rc.right  = pdp->rcBtn.left - 1;
    SECRecomputeSizing(&pdp->sec, &pdp->rc);
}

// deal with control codes
LRESULT DPHandleKeydown(DATEPICK *pdp, WPARAM wParam, LPARAM lParam)
{
    int delta = 1;
        
    if (wParam == VK_F4 && !pdp->fUseUpDown)
    {
        DPLBD_MonthCal(pdp, FALSE);
    }
    else if (DatePick_AppCanParse(pdp) && wParam == VK_F2)
    {
        DPHandleSECEdit(pdp);
    }        
    else if (pdp->fCheckFocus)
    {
        switch (wParam)
        {
        case VK_LEFT:
            delta = -1;
            // fall through...
        case VK_RIGHT:
            if (pdp->fCheck)
            {
                if (SUBEDIT_NONE != SECIncrFocus(pdp, delta))
                {
                    pdp->fCheckFocus = FALSE;
                    InvalidateRect(pdp->ci.hwnd, &pdp->rcCheck, TRUE);
                }
            }
            break;
        }
    }
    else
    {
        switch (wParam)
        {
        case VK_HOME:
            if (GetKeyState(VK_CONTROL) < 0)
            {
                SYSTEMTIME st;
                GetLocalTime(&st);
                DPSetDate(pdp, &st, TRUE);
                break;
            }
            // fall through...

        default:
            if (SECHandleKeydown(pdp, wParam, lParam))
            {
                DPNotifyDateChange(pdp);
            }
            else if (DatePick_ShowCheck(pdp))
            {
                if (pdp->sec.iseCur < 0)
                {
                    pdp->fCheckFocus = TRUE;
                    InvalidateRect(pdp->ci.hwnd, &pdp->rcCheck, TRUE);
                }
            }
            break;
        }
    }
    
    return(0);
}

// deal with characters
LRESULT DPHandleChar(DATEPICK *pdp, WPARAM wParam, LPARAM lParam)
{
    TCHAR ch = (TCHAR)wParam;

    if (pdp->fCheckFocus)
    {
        // this is the only character we care about in this case
        if (ch == TEXT(' '))
        {
            pdp->fCheck = 1-pdp->fCheck;
            InvalidateRect(pdp->ci.hwnd, NULL, TRUE);
            DPNotifyDateChange(pdp);
        }
        else
        {
            MessageBeep(MB_ICONHAND);
        }
    }
    else
    {
        // let the subedit handle this -- a value can change
        SECHandleChar(pdp, ch);
    }
    return(0);
}

void DPNotifyDateChange(DATEPICK *pdp)
{
    NMDATETIMECHANGE nmdc = {0};
    BOOL fChanged;

    if (pdp->fNoNotify)
        return;

    if (pdp->fCheck == 0)
    {
        nmdc.dwFlags = GDT_NONE;
    }
    else
    {
        // validate date - do it here in only one place
        if (CmpSystemtime(&pdp->sec.st, &pdp->stMin) < 0)
        {
            pdp->sec.st = pdp->stMin;
            InvalidateRect(pdp->ci.hwnd, NULL, TRUE);
            SECInvalidate(&pdp->sec, SE_APP);
        }
        else if (CmpSystemtime(&pdp->sec.st, &pdp->stMax) > 0)
        {
            pdp->sec.st = pdp->stMax;
            InvalidateRect(pdp->ci.hwnd, NULL, TRUE);
            SECInvalidate(&pdp->sec, SE_APP);
        }

        nmdc.dwFlags = GDT_VALID;
        SECGetSystemtime(&pdp->sec, &nmdc.st);
    }

    fChanged = CmpSystemtime(&pdp->stPrev, &nmdc.st);
    if (fChanged) {
        MyNotifyWinEvent(EVENT_OBJECT_NAMECHANGE, pdp->ci.hwnd, OBJID_WINDOW, INDEXID_CONTAINER);
    }

    //
    //  APP COMPAT:  IE4 always notified even if the date didn't change.
    //               I don't know of any apps that rely on this
    //               but I'm not gonna risk it.
    //
    if (fChanged || pdp->ci.iVersion < 5)
    {
        pdp->stPrev = nmdc.st;
        CCSendNotify(&pdp->ci, DTN_DATETIMECHANGE, &nmdc.nmhdr);
    }
}

BOOL DPSetDate(DATEPICK *pdp, SYSTEMTIME *pst, BOOL fMungeDate)
{
    BOOL fChanged = FALSE;

    // make sure that the new date is within the valid range
    if (CmpSystemtime(pst, &pdp->stMin) < 0)
    {
        if (!fMungeDate)
            return(FALSE);
        pst = &pdp->stMin;
    }
    if (CmpSystemtime(&pdp->stMax, pst) < 0)
    {
        if (!fMungeDate)
            return(FALSE);
        pst = &pdp->stMax;
    }

    if (fMungeDate)
    {
        // only copy the date portion
        CopyDate(*pst, pdp->sec.st);
        fChanged = TRUE;
    }
    else
    {
        fChanged = SECSetSystemtime(pdp, pst);
    }
    
    if (fChanged)
    {
        SECInvalidate(&pdp->sec, SE_APP); // SE_APP invalidates everything
        DPNotifyDateChange(pdp);   
    }
    
    return(TRUE);
}

void DPDrawDropdownButton(DATEPICK *pdp, HDC hdc, BOOL fPressed)
{
    UINT dfcs;
    
    dfcs = DFCS_SCROLLDOWN;
    if (fPressed)
        dfcs |= DFCS_PUSHED | DFCS_FLAT;
    if (!pdp->fEnabled)
        dfcs |= DFCS_INACTIVE;
    DrawFrameControl(hdc, &pdp->rcBtn, DFC_SCROLL, dfcs);
}
