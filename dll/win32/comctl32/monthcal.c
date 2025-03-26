/*
 * Month calendar control
 *
 * Copyright 1998, 1999 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * Copyright 1999 Alex Priem (alexp@sci.kun.nl)
 * Copyright 1999 Chris Morgan <cmorgan@wpi.edu> and
 *		  James Abbatiello <abbeyj@wpi.edu>
 * Copyright 2000 Uwe Bonnes <bon@elektron.ikp.physik.tu-darmstadt.de>
 * Copyright 2009-2011 Nikolay Sivov
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * TODO:
 *    -- MCM_[GS]ETUNICODEFORMAT
 *    -- handle resources better (doesn't work now); 
 *    -- take care of internationalization.
 *    -- keyboard handling.
 *    -- search for FIXME
 */

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "uxtheme.h"
#include "vssym32.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(monthcal);

/* FIXME: Inspect */
#define MCS_NOSELCHANGEONNAV 0x0100

#define MC_SEL_LBUTUP	    1	/* Left button released */
#define MC_SEL_LBUTDOWN	    2	/* Left button pressed in calendar */
#define MC_PREVPRESSED      4   /* Prev month button pressed */
#define MC_NEXTPRESSED      8   /* Next month button pressed */
#define MC_PREVNEXTMONTHDELAY   350	/* when continuously pressing `next/prev
					   month', wait 350 ms before going
					   to the next/prev month */
#define MC_TODAYUPDATEDELAY 120000 /* time between today check for update (2 min) */

#define MC_PREVNEXTMONTHTIMER   1	/* Timer IDs */
#define MC_TODAYUPDATETIMER     2

#define MC_CALENDAR_PADDING     6

/* convert from days to 100 nanoseconds unit - used as FILETIME unit */
#define DAYSTO100NSECS(days) (((ULONGLONG)(days))*24*60*60*10000000)

enum CachedPen
{
    PenRed = 0,
    PenText,
    PenLast
};

enum CachedBrush
{
    BrushTitle = 0,
    BrushMonth,
    BrushBackground,
    BrushLast
};

/* single calendar data */
typedef struct _CALENDAR_INFO
{
    RECT title;      /* rect for the header above the calendar */
    RECT titlemonth; /* the 'month name' text in the header */
    RECT titleyear;  /* the 'year number' text in the header */
    RECT wdays;      /* week days at top */
    RECT days;       /* calendar area */
    RECT weeknums;   /* week numbers at left side */

    SYSTEMTIME month;/* contains calendar main month/year */
} CALENDAR_INFO;

typedef struct
{
    HWND	hwndSelf;
    DWORD	dwStyle; /* cached GWL_STYLE */

    COLORREF    colors[MCSC_TRAILINGTEXT+1];
    HBRUSH      brushes[BrushLast];
    HPEN        pens[PenLast];

    HFONT	hFont;
    HFONT	hBoldFont;
    int		textHeight;
    int		height_increment;
    int		width_increment;
    INT		delta;	/* scroll rate; # of months that the */
                        /* control moves when user clicks a scroll button */
    int		firstDay;	/* Start month calendar with firstDay's day,
				   stored in SYSTEMTIME format */
    BOOL	firstDaySet;    /* first week day differs from locale defined */

    BOOL	isUnicode;      /* value set with MCM_SETUNICODE format */

    MONTHDAYSTATE *monthdayState;
    SYSTEMTIME	todaysDate;
    BOOL	todaySet;       /* Today was forced with MCM_SETTODAY */
    int		status;		/* See MC_SEL flags */
    SYSTEMTIME	firstSel;	/* first selected day */
    INT		maxSelCount;
    SYSTEMTIME	minSel;         /* contains single selection when used without MCS_MULTISELECT */
    SYSTEMTIME	maxSel;
    SYSTEMTIME  focusedSel;     /* date currently focused with mouse movement */
    DWORD	rangeValid;
    SYSTEMTIME	minDate;
    SYSTEMTIME	maxDate;

    RECT titlebtnnext;	/* the `next month' button in the header */
    RECT titlebtnprev;  /* the `prev month' button in the header */
    RECT todayrect;	/* `today: xx/xx/xx' text rect */
    HWND hwndNotify;    /* Window to receive the notifications */
    HWND hWndYearEdit;  /* Window Handle of edit box to handle years */
    HWND hWndYearUpDown;/* Window Handle of updown box to handle years */
    WNDPROC EditWndProc;  /* original Edit window procedure */

    CALENDAR_INFO *calendars;
    SIZE dim;           /* [cx,cy] - dimensions of calendars matrix, row/column count */
} MONTHCAL_INFO, *LPMONTHCAL_INFO;

static const WCHAR themeClass[] = { 'S','c','r','o','l','l','b','a','r',0 };

/* empty SYSTEMTIME const */
static const SYSTEMTIME st_null;
/* valid date limits */
static const SYSTEMTIME max_allowed_date = { /* wYear */ 9999, /* wMonth */ 12, /* wDayOfWeek */ 0, /* wDay */ 31 };
static const SYSTEMTIME min_allowed_date = { /* wYear */ 1752, /* wMonth */ 9,  /* wDayOfWeek */ 0, /* wDay */ 14 };

/* Prev/Next buttons */
enum nav_direction
{
    DIRECTION_BACKWARD,
    DIRECTION_FORWARD
};

/* helper functions  */
static inline INT MONTHCAL_GetCalCount(const MONTHCAL_INFO *infoPtr)
{
   return infoPtr->dim.cx * infoPtr->dim.cy;
}

/* send a single MCN_SELCHANGE notification */
static inline void MONTHCAL_NotifySelectionChange(const MONTHCAL_INFO *infoPtr)
{
    NMSELCHANGE nmsc;

    nmsc.nmhdr.hwndFrom = infoPtr->hwndSelf;
    nmsc.nmhdr.idFrom   = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
    nmsc.nmhdr.code     = MCN_SELCHANGE;
    nmsc.stSelStart     = infoPtr->minSel;
    nmsc.stSelStart.wDayOfWeek = 0;
    if(infoPtr->dwStyle & MCS_MULTISELECT){
        nmsc.stSelEnd = infoPtr->maxSel;
        nmsc.stSelEnd.wDayOfWeek = 0;
    }
    else
        nmsc.stSelEnd = st_null;

    SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, nmsc.nmhdr.idFrom, (LPARAM)&nmsc);
}

/* send a single MCN_SELECT notification */
static inline void MONTHCAL_NotifySelect(const MONTHCAL_INFO *infoPtr)
{
    NMSELCHANGE nmsc;

    nmsc.nmhdr.hwndFrom = infoPtr->hwndSelf;
    nmsc.nmhdr.idFrom   = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
    nmsc.nmhdr.code     = MCN_SELECT;
    nmsc.stSelStart     = infoPtr->minSel;
    nmsc.stSelStart.wDayOfWeek = 0;
    if(infoPtr->dwStyle & MCS_MULTISELECT){
        nmsc.stSelEnd = infoPtr->maxSel;
        nmsc.stSelEnd.wDayOfWeek = 0;
    }
    else
        nmsc.stSelEnd = st_null;

    SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, nmsc.nmhdr.idFrom, (LPARAM)&nmsc);
}

static inline int MONTHCAL_MonthDiff(const SYSTEMTIME *left, const SYSTEMTIME *right)
{
    return (right->wYear - left->wYear)*12 + right->wMonth - left->wMonth;
}

/* returns the number of days in any given month, checking for leap days */
/* January is 1, December is 12 */
int MONTHCAL_MonthLength(int month, int year)
{
  static const int mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  /* Wrap around, this eases handling. Getting length only we shouldn't care
     about year change here cause January and December have
     the same day quantity */
  if(month == 0)
    month = 12;
  else if(month == 13)
    month = 1;

  /* special case for calendar transition year */
  if(month == min_allowed_date.wMonth && year == min_allowed_date.wYear) return 19;

  /* if we have a leap year add 1 day to February */
  /* a leap year is a year either divisible by 400 */
  /* or divisible by 4 and not by 100 */
  if(month == 2) { /* February */
    return mdays[month - 1] + ((year%400 == 0) ? 1 : ((year%100 != 0) &&
     (year%4 == 0)) ? 1 : 0);
  }
  else {
    return mdays[month - 1];
  }
}

/* compares timestamps using date part only */
static inline BOOL MONTHCAL_IsDateEqual(const SYSTEMTIME *first, const SYSTEMTIME *second)
{
  return (first->wYear == second->wYear) && (first->wMonth == second->wMonth) &&
         (first->wDay  == second->wDay);
}

/* make sure that date fields are valid */
static BOOL MONTHCAL_ValidateDate(const SYSTEMTIME *time)
{
    if (time->wMonth < 1 || time->wMonth > 12 )
        return FALSE;
    if (time->wDay == 0 || time->wDay > MONTHCAL_MonthLength(time->wMonth, time->wYear))
        return FALSE;

    return TRUE;
}

/* Copies timestamp part only.
 *
 * PARAMETERS
 *
 *  [I] from : source date
 *  [O] to   : dest date
 */
static void MONTHCAL_CopyTime(const SYSTEMTIME *from, SYSTEMTIME *to)
{
  to->wHour   = from->wHour;
  to->wMinute = from->wMinute;
  to->wSecond = from->wSecond;
}

/* Copies date part only.
 *
 * PARAMETERS
 *
 *  [I] from : source date
 *  [O] to   : dest date
 */
static void MONTHCAL_CopyDate(const SYSTEMTIME *from, SYSTEMTIME *to)
{
  to->wYear  = from->wYear;
  to->wMonth = from->wMonth;
  to->wDay   = from->wDay;
  to->wDayOfWeek = from->wDayOfWeek;
}

/* Compares two dates in SYSTEMTIME format
 *
 * PARAMETERS
 *
 *  [I] first  : pointer to valid first date data to compare
 *  [I] second : pointer to valid second date data to compare
 *
 * RETURN VALUE
 *
 *  -1 : first <  second
 *   0 : first == second
 *   1 : first >  second
 *
 *  Note that no date validation performed, already validated values expected.
 */
LONG MONTHCAL_CompareSystemTime(const SYSTEMTIME *first, const SYSTEMTIME *second)
{
  FILETIME ft_first, ft_second;

  SystemTimeToFileTime(first, &ft_first);
  SystemTimeToFileTime(second, &ft_second);

  return CompareFileTime(&ft_first, &ft_second);
}

static LONG MONTHCAL_CompareMonths(const SYSTEMTIME *first, const SYSTEMTIME *second)
{
  SYSTEMTIME st_first, st_second;

  st_first = st_second = st_null;
  MONTHCAL_CopyDate(first, &st_first);
  MONTHCAL_CopyDate(second, &st_second);
  st_first.wDay = st_second.wDay = 1;

  return MONTHCAL_CompareSystemTime(&st_first, &st_second);
}

static LONG MONTHCAL_CompareDate(const SYSTEMTIME *first, const SYSTEMTIME *second)
{
  SYSTEMTIME st_first, st_second;

  st_first = st_second = st_null;
  MONTHCAL_CopyDate(first, &st_first);
  MONTHCAL_CopyDate(second, &st_second);

  return MONTHCAL_CompareSystemTime(&st_first, &st_second);
}

/* Checks largest possible date range and configured one
 *
 * PARAMETERS
 *
 *  [I] infoPtr : valid pointer to control data
 *  [I] date    : pointer to valid date data to check
 *  [I] fix     : make date fit valid range
 *
 * RETURN VALUE
 *
 *  TRUE  - date within largest and configured range
 *  FALSE - date is outside largest or configured range
 */
static BOOL MONTHCAL_IsDateInValidRange(const MONTHCAL_INFO *infoPtr,
                                        SYSTEMTIME *date, BOOL fix)
{
  const SYSTEMTIME *fix_st = NULL;

  if(MONTHCAL_CompareSystemTime(date, &max_allowed_date) == 1) {
     fix_st = &max_allowed_date;
  }
  else if(MONTHCAL_CompareSystemTime(date, &min_allowed_date) == -1) {
     fix_st = &min_allowed_date;
  }
  else {
     if(infoPtr->rangeValid & GDTR_MAX) {
        if((MONTHCAL_CompareSystemTime(date, &infoPtr->maxDate) == 1)) {
           fix_st = &infoPtr->maxDate;
        }
     }

     if(infoPtr->rangeValid & GDTR_MIN) {
        if((MONTHCAL_CompareSystemTime(date, &infoPtr->minDate) == -1)) {
           fix_st = &infoPtr->minDate;
        }
     }
  }

  if (fix && fix_st) {
    date->wYear  = fix_st->wYear;
    date->wMonth = fix_st->wMonth;
  }

  return !fix_st;
}

/* Checks passed range width with configured maximum selection count
 *
 * PARAMETERS
 *
 *  [I] infoPtr : valid pointer to control data
 *  [I] range0  : pointer to valid date data (requested bound)
 *  [I] range1  : pointer to valid date data (primary bound)
 *  [O] adjust  : returns adjusted range bound to fit maximum range (optional)
 *
 *  Adjust value computed basing on primary bound and current maximum selection
 *  count. For simple range check (without adjusted value required) (range0, range1)
 *  relation means nothing.
 *
 * RETURN VALUE
 *
 *  TRUE  - range is shorter or equal to maximum
 *  FALSE - range is larger than maximum
 */
static BOOL MONTHCAL_IsSelRangeValid(const MONTHCAL_INFO *infoPtr,
                                     const SYSTEMTIME *range0,
                                     const SYSTEMTIME *range1,
                                     SYSTEMTIME *adjust)
{
  ULARGE_INTEGER ul_range0, ul_range1, ul_diff;
  FILETIME ft_range0, ft_range1;
  LONG cmp;

  SystemTimeToFileTime(range0, &ft_range0);
  SystemTimeToFileTime(range1, &ft_range1);

  ul_range0.u.LowPart  = ft_range0.dwLowDateTime;
  ul_range0.u.HighPart = ft_range0.dwHighDateTime;
  ul_range1.u.LowPart  = ft_range1.dwLowDateTime;
  ul_range1.u.HighPart = ft_range1.dwHighDateTime;

  cmp = CompareFileTime(&ft_range0, &ft_range1);

  if(cmp == 1)
     ul_diff.QuadPart = ul_range0.QuadPart - ul_range1.QuadPart;
  else
     ul_diff.QuadPart = -ul_range0.QuadPart + ul_range1.QuadPart;

  if(ul_diff.QuadPart >= DAYSTO100NSECS(infoPtr->maxSelCount)) {

     if(adjust) {
       if(cmp == 1)
          ul_range0.QuadPart = ul_range1.QuadPart + DAYSTO100NSECS(infoPtr->maxSelCount - 1);
       else
          ul_range0.QuadPart = ul_range1.QuadPart - DAYSTO100NSECS(infoPtr->maxSelCount - 1);

       ft_range0.dwLowDateTime  = ul_range0.u.LowPart;
       ft_range0.dwHighDateTime = ul_range0.u.HighPart;
       FileTimeToSystemTime(&ft_range0, adjust);
     }

     return FALSE;
  }
  else return TRUE;
}

/* Used in MCM_SETRANGE/MCM_SETSELRANGE to determine resulting time part.
   Milliseconds are intentionally not validated. */
static BOOL MONTHCAL_ValidateTime(const SYSTEMTIME *time)
{
  if((time->wHour > 24) || (time->wMinute > 59) || (time->wSecond > 59))
    return FALSE;
  else
    return TRUE;
}

/* Note:Depending on DST, this may be offset by a day.
   Need to find out if we're on a DST place & adjust the clock accordingly.
   Above function assumes we have a valid data.
   Valid for year>1752;  1 <= d <= 31, 1 <= m <= 12.
   0 = Sunday.
*/

/* Returns the day in the week
 *
 * PARAMETERS
 *  [i] date    : input date
 *  [I] inplace : set calculated value back to date structure
 *
 * RETURN VALUE
 *   day of week in SYSTEMTIME format: (0 == sunday,..., 6 == saturday)
 */
int MONTHCAL_CalculateDayOfWeek(SYSTEMTIME *date, BOOL inplace)
{
  SYSTEMTIME st = st_null;
  FILETIME ft;

  MONTHCAL_CopyDate(date, &st);

  SystemTimeToFileTime(&st, &ft);
  FileTimeToSystemTime(&ft, &st);

  if (inplace) date->wDayOfWeek = st.wDayOfWeek;

  return st.wDayOfWeek;
}

/* add/subtract 'months' from date */
static inline void MONTHCAL_GetMonth(SYSTEMTIME *date, INT months)
{
  INT length, m = date->wMonth + months;

  date->wYear += m > 0 ? (m - 1) / 12 : m / 12 - 1;
  date->wMonth = m > 0 ? (m - 1) % 12 + 1 : 12 + m % 12;
  /* fix moving from last day in a month */
  length = MONTHCAL_MonthLength(date->wMonth, date->wYear);
  if(date->wDay > length) date->wDay = length;
  MONTHCAL_CalculateDayOfWeek(date, TRUE);
}

/* properly updates date to point on next month */
static inline void MONTHCAL_GetNextMonth(SYSTEMTIME *date)
{
  MONTHCAL_GetMonth(date, 1);
}

/* properly updates date to point on prev month */
static inline void MONTHCAL_GetPrevMonth(SYSTEMTIME *date)
{
  MONTHCAL_GetMonth(date, -1);
}

/* Returns full date for a first currently visible day */
static void MONTHCAL_GetMinDate(const MONTHCAL_INFO *infoPtr, SYSTEMTIME *date)
{
  /* zero indexed calendar has the earliest date */
  SYSTEMTIME st_first = infoPtr->calendars[0].month;
  INT firstDay;

  st_first.wDay = 1;
  firstDay = MONTHCAL_CalculateDayOfWeek(&st_first, FALSE);

  *date = infoPtr->calendars[0].month;
  MONTHCAL_GetPrevMonth(date);

  date->wDay = MONTHCAL_MonthLength(date->wMonth, date->wYear) +
               (infoPtr->firstDay - firstDay) % 7 + 1;

  if(date->wDay > MONTHCAL_MonthLength(date->wMonth, date->wYear))
    date->wDay -= 7;

  /* fix day of week */
  MONTHCAL_CalculateDayOfWeek(date, TRUE);
}

/* Returns full date for a last currently visible day */
static void MONTHCAL_GetMaxDate(const MONTHCAL_INFO *infoPtr, SYSTEMTIME *date)
{
  /* the latest date is in latest calendar */
  SYSTEMTIME st, *lt_month = &infoPtr->calendars[MONTHCAL_GetCalCount(infoPtr)-1].month;
  INT first_day;

  *date = *lt_month;
  st = *lt_month;

  /* day of week of first day of current month */
  st.wDay = 1;
  first_day = MONTHCAL_CalculateDayOfWeek(&st, FALSE);

  MONTHCAL_GetNextMonth(date);
  MONTHCAL_GetPrevMonth(&st);

  /* last calendar starts with some date from previous month that not displayed */
  st.wDay = MONTHCAL_MonthLength(st.wMonth, st.wYear) +
             (infoPtr->firstDay - first_day) % 7 + 1;
  if (st.wDay > MONTHCAL_MonthLength(st.wMonth, st.wYear)) st.wDay -= 7;

  /* Use month length to get max day. 42 means max day count in calendar area */
  date->wDay = 42 - (MONTHCAL_MonthLength(st.wMonth, st.wYear) - st.wDay + 1) -
                     MONTHCAL_MonthLength(lt_month->wMonth, lt_month->wYear);

  /* fix day of week */
  MONTHCAL_CalculateDayOfWeek(date, TRUE);
}

/* From a given point calculate the row, column and day in the calendar,
   'day == 0' means the last day of the last month. */
static int MONTHCAL_GetDayFromPos(const MONTHCAL_INFO *infoPtr, POINT pt, INT calIdx)
{
  SYSTEMTIME st = infoPtr->calendars[calIdx].month;
  int firstDay, col, row;
  RECT client;

  GetClientRect(infoPtr->hwndSelf, &client);

  /* if the point is outside the x bounds of the window put it at the boundary */
  if (pt.x > client.right) pt.x = client.right;

  col = (pt.x - infoPtr->calendars[calIdx].days.left ) / infoPtr->width_increment;
  row = (pt.y - infoPtr->calendars[calIdx].days.top  ) / infoPtr->height_increment;

  st.wDay = 1;
  firstDay = (MONTHCAL_CalculateDayOfWeek(&st, FALSE) + 6 - infoPtr->firstDay) % 7;
  return col + 7 * row - firstDay;
}

/* Get day position for given date and calendar
 *
 * PARAMETERS
 *
 *  [I] infoPtr : pointer to control data
 *  [I] date : date value
 *  [O] col : day column (zero based)
 *  [O] row : week column (zero based)
 *  [I] calIdx : calendar index
 */
static void MONTHCAL_GetDayPos(const MONTHCAL_INFO *infoPtr, const SYSTEMTIME *date,
    INT *col, INT *row, INT calIdx)
{
  SYSTEMTIME st = infoPtr->calendars[calIdx].month;
  INT first;

  st.wDay = 1;
  first = (MONTHCAL_CalculateDayOfWeek(&st, FALSE) + 6 - infoPtr->firstDay) % 7;

  if (calIdx == 0 || calIdx == MONTHCAL_GetCalCount(infoPtr)-1) {
      const SYSTEMTIME *cal = &infoPtr->calendars[calIdx].month;
      LONG cmp = MONTHCAL_CompareMonths(date, &st);

      /* previous month */
      if (cmp == -1) {
        *col = (first - MONTHCAL_MonthLength(date->wMonth, cal->wYear) + date->wDay) % 7;
        *row = 0;
        return;
      }

      /* next month calculation is same as for current, just add current month length */
      if (cmp == 1)
          first += MONTHCAL_MonthLength(cal->wMonth, cal->wYear);
  }

  *col = (date->wDay + first) % 7;
  *row = (date->wDay + first - *col) / 7;
}

/* returns bounding box for day in given position in given calendar */
static inline void MONTHCAL_GetDayRectI(const MONTHCAL_INFO *infoPtr, RECT *r,
  INT col, INT row, INT calIdx)
{
  r->left   = infoPtr->calendars[calIdx].days.left + col * infoPtr->width_increment;
  r->right  = r->left + infoPtr->width_increment;
  r->top    = infoPtr->calendars[calIdx].days.top  + row * infoPtr->height_increment;
  r->bottom = r->top + infoPtr->textHeight;
}

/* Returns bounding box for given date
 *
 * NOTE: when calendar index is unknown pass -1
 */
static BOOL MONTHCAL_GetDayRect(const MONTHCAL_INFO *infoPtr, const SYSTEMTIME *date, RECT *r, INT calIdx)
{
  INT col, row;

  if (!MONTHCAL_ValidateDate(date))
  {
      SetRectEmpty(r);
      return FALSE;
  }

  if (calIdx == -1)
  {
      INT cmp = MONTHCAL_CompareMonths(date, &infoPtr->calendars[0].month);

      if (cmp <= 0)
          calIdx = 0;
      else
      {
          cmp = MONTHCAL_CompareMonths(date, &infoPtr->calendars[MONTHCAL_GetCalCount(infoPtr)-1].month);
          if (cmp >= 0)
              calIdx = MONTHCAL_GetCalCount(infoPtr)-1;
          else
          {
              for (calIdx = 1; calIdx < MONTHCAL_GetCalCount(infoPtr)-1; calIdx++)
                  if (MONTHCAL_CompareMonths(date, &infoPtr->calendars[calIdx].month) == 0)
                      break;
          }
      }
  }

  MONTHCAL_GetDayPos(infoPtr, date, &col, &row, calIdx);
  MONTHCAL_GetDayRectI(infoPtr, r, col, row, calIdx);

  return TRUE;
}

static LRESULT
MONTHCAL_GetMonthRange(const MONTHCAL_INFO *infoPtr, DWORD flag, SYSTEMTIME *st)
{
  INT range;

  TRACE("flag=%d, st=%p\n", flag, st);

  switch (flag) {
  case GMR_VISIBLE:
  {
      if (st)
      {
          st[0] = infoPtr->calendars[0].month;
          st[1] = infoPtr->calendars[MONTHCAL_GetCalCount(infoPtr)-1].month;

          if (st[0].wMonth == min_allowed_date.wMonth &&
              st[0].wYear  == min_allowed_date.wYear)
          {
              st[0].wDay = min_allowed_date.wDay;
          }
          else
              st[0].wDay = 1;
          MONTHCAL_CalculateDayOfWeek(&st[0], TRUE);

          st[1].wDay = MONTHCAL_MonthLength(st[1].wMonth, st[1].wYear);
          MONTHCAL_CalculateDayOfWeek(&st[1], TRUE);
      }

      range = MONTHCAL_GetCalCount(infoPtr);
      break;
  }
  case GMR_DAYSTATE:
  {
      if (st)
      {
          MONTHCAL_GetMinDate(infoPtr, &st[0]);
          MONTHCAL_GetMaxDate(infoPtr, &st[1]);
      }
      /* include two partially visible months */
      range = MONTHCAL_GetCalCount(infoPtr) + 2;
      break;
  }
  default:
      WARN("Unknown flag value, got %d\n", flag);
      range = 0;
  }

  return range;
}

/* Focused day helper:

   - set focused date to given value;
   - reset to zero value if NULL passed;
   - invalidate previous and new day rectangle only if needed.

   Returns TRUE if focused day changed, FALSE otherwise.
*/
static BOOL MONTHCAL_SetDayFocus(MONTHCAL_INFO *infoPtr, const SYSTEMTIME *st)
{
  RECT r;

  if(st)
  {
    /* there's nothing to do if it's the same date,
       mouse move within same date rectangle case */
    if(MONTHCAL_IsDateEqual(&infoPtr->focusedSel, st)) return FALSE;

    /* invalidate old focused day */
    if (MONTHCAL_GetDayRect(infoPtr, &infoPtr->focusedSel, &r, -1))
      InvalidateRect(infoPtr->hwndSelf, &r, FALSE);

    infoPtr->focusedSel = *st;
  }

  /* On set invalidates new day, on reset clears previous focused day. */
  if (MONTHCAL_GetDayRect(infoPtr, &infoPtr->focusedSel, &r, -1))
    InvalidateRect(infoPtr->hwndSelf, &r, FALSE);

  if(!st && MONTHCAL_ValidateDate(&infoPtr->focusedSel))
    infoPtr->focusedSel = st_null;

  return TRUE;
}

/* draw today boundary box for specified rectangle */
static void MONTHCAL_Circle(const MONTHCAL_INFO *infoPtr, HDC hdc, const RECT *r)
{
  HPEN old_pen = SelectObject(hdc, infoPtr->pens[PenRed]);
  HBRUSH old_brush;

  old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
  Rectangle(hdc, r->left, r->top, r->right, r->bottom);

  SelectObject(hdc, old_brush);
  SelectObject(hdc, old_pen);
}

/* Draw today day mark rectangle
 *
 * [I] hdc  : context to draw in
 * [I] date : day to mark with rectangle
 *
 */
static void MONTHCAL_CircleDay(const MONTHCAL_INFO *infoPtr, HDC hdc,
                               const SYSTEMTIME *date)
{
  RECT r;

  MONTHCAL_GetDayRect(infoPtr, date, &r, -1);
  MONTHCAL_Circle(infoPtr, hdc, &r);
}

static void MONTHCAL_DrawDay(const MONTHCAL_INFO *infoPtr, HDC hdc, const SYSTEMTIME *st,
                             int bold, const PAINTSTRUCT *ps)
{
  static const WCHAR fmtW[] = { '%','d',0 };
  WCHAR buf[10];
  RECT r, r_temp;
  COLORREF oldCol = 0;
  COLORREF oldBk  = 0;
  INT old_bkmode, selection;

  /* no need to check styles: when selection is not valid, it is set to zero.
     1 < day < 31, so everything is OK */
  MONTHCAL_GetDayRect(infoPtr, st, &r, -1);
  if(!IntersectRect(&r_temp, &(ps->rcPaint), &r)) return;

  if ((MONTHCAL_CompareDate(st, &infoPtr->minSel) >= 0) &&
      (MONTHCAL_CompareDate(st, &infoPtr->maxSel) <= 0))
  {
    TRACE("%d %d %d\n", st->wDay, infoPtr->minSel.wDay, infoPtr->maxSel.wDay);
    TRACE("%s\n", wine_dbgstr_rect(&r));
    oldCol = SetTextColor(hdc, infoPtr->colors[MCSC_MONTHBK]);
    oldBk = SetBkColor(hdc, infoPtr->colors[MCSC_TRAILINGTEXT]);
    FillRect(hdc, &r, infoPtr->brushes[BrushTitle]);

    selection = 1;
  }
  else
    selection = 0;

  SelectObject(hdc, bold ? infoPtr->hBoldFont : infoPtr->hFont);

  old_bkmode = SetBkMode(hdc, TRANSPARENT);
  wsprintfW(buf, fmtW, st->wDay);
  DrawTextW(hdc, buf, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
  SetBkMode(hdc, old_bkmode);

  if (selection)
  {
    SetTextColor(hdc, oldCol);
    SetBkColor(hdc, oldBk);
  }
}

static void MONTHCAL_PaintButton(MONTHCAL_INFO *infoPtr, HDC hdc, enum nav_direction button)
{
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
    RECT *r = button == DIRECTION_FORWARD ? &infoPtr->titlebtnnext : &infoPtr->titlebtnprev;
    BOOL pressed = button == DIRECTION_FORWARD ? infoPtr->status & MC_NEXTPRESSED :
                                                 infoPtr->status & MC_PREVPRESSED;
    if (theme)
    {
        static const int states[] = {
            /* Prev button */
            ABS_LEFTNORMAL,  ABS_LEFTPRESSED,  ABS_LEFTDISABLED,
            /* Next button */
            ABS_RIGHTNORMAL, ABS_RIGHTPRESSED, ABS_RIGHTDISABLED
        };
        int stateNum = button == DIRECTION_FORWARD ? 3 : 0;
        if (pressed)
            stateNum += 1;
        else
        {
            if (infoPtr->dwStyle & WS_DISABLED) stateNum += 2;
        }
        DrawThemeBackground (theme, hdc, SBP_ARROWBTN, states[stateNum], r, NULL);
    }
    else
    {
        int style = button == DIRECTION_FORWARD ? DFCS_SCROLLRIGHT : DFCS_SCROLLLEFT;
        if (pressed)
            style |= DFCS_PUSHED;
        else
        {
            if (infoPtr->dwStyle & WS_DISABLED) style |= DFCS_INACTIVE;
        }
        
        DrawFrameControl(hdc, r, DFC_SCROLL, style);
    }
}

/* paint a title with buttons and month/year string */
static void MONTHCAL_PaintTitle(MONTHCAL_INFO *infoPtr, HDC hdc, const PAINTSTRUCT *ps, INT calIdx)
{
  static const WCHAR mmmmW[] = {'M','M','M','M',0};
  static const WCHAR mmmW[] = {'M','M','M',0};
  static const WCHAR mmW[] = {'M','M',0};
  static const WCHAR fmtyearW[] = {'%','l','d',0};
  static const WCHAR fmtmmW[] = {'%','0','2','d',0};
  static const WCHAR fmtmW[] = {'%','d',0};
  RECT *title = &infoPtr->calendars[calIdx].title;
  const SYSTEMTIME *st = &infoPtr->calendars[calIdx].month;
  WCHAR monthW[80], strW[80], fmtW[80], yearW[6] /* valid year range is 1601-30827 */;
  int yearoffset, monthoffset, shiftX;
  SIZE sz;

  /* fill header box */
  FillRect(hdc, title, infoPtr->brushes[BrushTitle]);

  /* month/year string */
  SetBkColor(hdc, infoPtr->colors[MCSC_TITLEBK]);
  SetTextColor(hdc, infoPtr->colors[MCSC_TITLETEXT]);
  SelectObject(hdc, infoPtr->hBoldFont);

  /* draw formatted date string */
  GetDateFormatW(LOCALE_USER_DEFAULT, DATE_YEARMONTH, st, NULL, strW, ARRAY_SIZE(strW));
  DrawTextW(hdc, strW, lstrlenW(strW), title, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SYEARMONTH, fmtW, ARRAY_SIZE(fmtW));
  wsprintfW(yearW, fmtyearW, st->wYear);

  /* month is trickier as it's possible to have different format pictures, we'll
     test for M, MM, MMM, and MMMM */
  if (wcsstr(fmtW, mmmmW))
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHNAME1+st->wMonth-1, monthW, ARRAY_SIZE(monthW));
  else if (wcsstr(fmtW, mmmW))
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SABBREVMONTHNAME1+st->wMonth-1, monthW, ARRAY_SIZE(monthW));
  else if (wcsstr(fmtW, mmW))
    wsprintfW(monthW, fmtmmW, st->wMonth);
  else
    wsprintfW(monthW, fmtmW, st->wMonth);

  /* update hit boxes */
  yearoffset = 0;
  while (strW[yearoffset])
  {
    if (!wcsncmp(&strW[yearoffset], yearW, lstrlenW(yearW)))
        break;
    yearoffset++;
  }

  monthoffset = 0;
  while (strW[monthoffset])
  {
    if (!wcsncmp(&strW[monthoffset], monthW, lstrlenW(monthW)))
        break;
    monthoffset++;
  }

  /* for left limits use offsets */
  sz.cx = 0;
  if (yearoffset)
    GetTextExtentPoint32W(hdc, strW, yearoffset, &sz);
  infoPtr->calendars[calIdx].titleyear.left = sz.cx;

  sz.cx = 0;
  if (monthoffset)
    GetTextExtentPoint32W(hdc, strW, monthoffset, &sz);
  infoPtr->calendars[calIdx].titlemonth.left = sz.cx;

  /* for right limits use actual string parts lengths */
  GetTextExtentPoint32W(hdc, &strW[yearoffset], lstrlenW(yearW), &sz);
  infoPtr->calendars[calIdx].titleyear.right = infoPtr->calendars[calIdx].titleyear.left + sz.cx;

  GetTextExtentPoint32W(hdc, monthW, lstrlenW(monthW), &sz);
  infoPtr->calendars[calIdx].titlemonth.right = infoPtr->calendars[calIdx].titlemonth.left + sz.cx;

  /* Finally translate rectangles to match center aligned string,
     hit rectangles are relative to title rectangle before translation. */
  GetTextExtentPoint32W(hdc, strW, lstrlenW(strW), &sz);
  shiftX = (title->right - title->left - sz.cx) / 2 + title->left;
  OffsetRect(&infoPtr->calendars[calIdx].titleyear, shiftX, 0);
  OffsetRect(&infoPtr->calendars[calIdx].titlemonth, shiftX, 0);
}

static void MONTHCAL_PaintWeeknumbers(const MONTHCAL_INFO *infoPtr, HDC hdc, const PAINTSTRUCT *ps, INT calIdx)
{
  const SYSTEMTIME *date = &infoPtr->calendars[calIdx].month;
  static const WCHAR fmt_weekW[] = { '%','d',0 };
  INT mindays, weeknum, weeknum1, startofprescal;
  INT i, prev_month;
  SYSTEMTIME st;
  WCHAR buf[80];
  HPEN old_pen;
  RECT r;

  if (!(infoPtr->dwStyle & MCS_WEEKNUMBERS)) return;

  MONTHCAL_GetMinDate(infoPtr, &st);
  startofprescal = st.wDay;
  st = *date;

  prev_month = date->wMonth - 1;
  if(prev_month == 0) prev_month = 12;

  /*
     Rules what week to call the first week of a new year:
     LOCALE_IFIRSTWEEKOFYEAR == 0 (e.g US?):
     The week containing Jan 1 is the first week of year
     LOCALE_IFIRSTWEEKOFYEAR == 2 (e.g. Germany):
     First week of year must contain 4 days of the new year
     LOCALE_IFIRSTWEEKOFYEAR == 1  (what countries?)
     The first week of the year must contain only days of the new year
  */
  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IFIRSTWEEKOFYEAR, buf, ARRAY_SIZE(buf));
  weeknum = wcstol(buf, NULL, 10);
  switch (weeknum)
  {
    case 1: mindays = 6;
	break;
    case 2: mindays = 3;
	break;
    case 0: mindays = 0;
        break;
    default:
        WARN("Unknown LOCALE_IFIRSTWEEKOFYEAR value %d, defaulting to 0\n", weeknum);
	mindays = 0;
  }

  if (date->wMonth == 1)
  {
    /* calculate all those exceptions for January */
    st.wDay = st.wMonth = 1;
    weeknum1 = MONTHCAL_CalculateDayOfWeek(&st, FALSE);
    if ((infoPtr->firstDay - weeknum1) % 7 > mindays)
	weeknum = 1;
    else
    {
	weeknum = 0;
	for(i = 0; i < 11; i++)
	   weeknum += MONTHCAL_MonthLength(i+1, date->wYear - 1);

	weeknum  += startofprescal + 7;
	weeknum  /= 7;
	st.wYear -= 1;
	weeknum1  = MONTHCAL_CalculateDayOfWeek(&st, FALSE);
	if ((infoPtr->firstDay - weeknum1) % 7 > mindays) weeknum++;
    }
  }
  else
  {
    weeknum = 0;
    for(i = 0; i < prev_month - 1; i++)
	weeknum += MONTHCAL_MonthLength(i+1, date->wYear);

    weeknum += startofprescal + 7;
    weeknum /= 7;
    st.wDay = st.wMonth = 1;
    weeknum1 = MONTHCAL_CalculateDayOfWeek(&st, FALSE);
    if ((infoPtr->firstDay - weeknum1) % 7 > mindays) weeknum++;
  }

  r = infoPtr->calendars[calIdx].weeknums;

  /* erase whole week numbers area */
  FillRect(hdc, &r, infoPtr->brushes[BrushMonth]);
  SetTextColor(hdc, infoPtr->colors[MCSC_TITLEBK]);

  /* reduce rectangle to one week number */
  r.bottom = r.top + infoPtr->height_increment;

  for(i = 0; i < 6; i++) {
    if((i == 0) && (weeknum > 50))
    {
        wsprintfW(buf, fmt_weekW, weeknum);
        weeknum = 0;
    }
    else if((i == 5) && (weeknum > 47))
    {
	wsprintfW(buf, fmt_weekW, 1);
    }
    else
	wsprintfW(buf, fmt_weekW, weeknum + i);

    DrawTextW(hdc, buf, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    OffsetRect(&r, 0, infoPtr->height_increment);
  }

  /* line separator for week numbers column */
  old_pen = SelectObject(hdc, infoPtr->pens[PenText]);
  MoveToEx(hdc, infoPtr->calendars[calIdx].weeknums.right, infoPtr->calendars[calIdx].weeknums.top + 3 , NULL);
  LineTo(hdc,   infoPtr->calendars[calIdx].weeknums.right, infoPtr->calendars[calIdx].weeknums.bottom);
  SelectObject(hdc, old_pen);
}

/* bottom today date */
static void MONTHCAL_PaintTodayTitle(const MONTHCAL_INFO *infoPtr, HDC hdc, const PAINTSTRUCT *ps)
{
  static const WCHAR fmt_todayW[] = { '%','s',' ','%','s',0 };
  WCHAR buf_todayW[30], buf_dateW[20], buf[80];
  RECT text_rect, box_rect;
  HFONT old_font;
  INT col;

  if(infoPtr->dwStyle & MCS_NOTODAY) return;

  LoadStringW(COMCTL32_hModule, IDM_TODAY, buf_todayW, ARRAY_SIZE(buf_todayW));
  col = infoPtr->dwStyle & MCS_NOTODAYCIRCLE ? 0 : 1;
  if (infoPtr->dwStyle & MCS_WEEKNUMBERS) col--;
  /* label is located below first calendar last row */
  MONTHCAL_GetDayRectI(infoPtr, &text_rect, col, 6, infoPtr->dim.cx * infoPtr->dim.cy - infoPtr->dim.cx);
  box_rect = text_rect;

  GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &infoPtr->todaysDate, NULL, buf_dateW, ARRAY_SIZE(buf_dateW));
  old_font = SelectObject(hdc, infoPtr->hBoldFont);
  SetTextColor(hdc, infoPtr->colors[MCSC_TEXT]);

  wsprintfW(buf, fmt_todayW, buf_todayW, buf_dateW);
  DrawTextW(hdc, buf, -1, &text_rect, DT_CALCRECT | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  DrawTextW(hdc, buf, -1, &text_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

  if(!(infoPtr->dwStyle & MCS_NOTODAYCIRCLE)) {
    OffsetRect(&box_rect, -infoPtr->width_increment, 0);
    MONTHCAL_Circle(infoPtr, hdc, &box_rect);
  }

  SelectObject(hdc, old_font);
}

/* today mark + focus */
static void MONTHCAL_PaintFocusAndCircle(const MONTHCAL_INFO *infoPtr, HDC hdc, const PAINTSTRUCT *ps)
{
  /* circle today date if only it's in fully visible month */
  if (!(infoPtr->dwStyle & MCS_NOTODAYCIRCLE))
  {
    INT i;

    for (i = 0; i < MONTHCAL_GetCalCount(infoPtr); i++)
      if (!MONTHCAL_CompareMonths(&infoPtr->todaysDate, &infoPtr->calendars[i].month))
      {
        MONTHCAL_CircleDay(infoPtr, hdc, &infoPtr->todaysDate);
        break;
      }
  }

  if (!MONTHCAL_IsDateEqual(&infoPtr->focusedSel, &st_null))
  {
    RECT r;
    MONTHCAL_GetDayRect(infoPtr, &infoPtr->focusedSel, &r, -1);
    DrawFocusRect(hdc, &r);
  }
}

/* months before first calendar month and after last calendar month */
static void MONTHCAL_PaintLeadTrailMonths(const MONTHCAL_INFO *infoPtr, HDC hdc, const PAINTSTRUCT *ps)
{
  INT mask, index;
  UINT length;
  SYSTEMTIME st_max, st;

  if (infoPtr->dwStyle & MCS_NOTRAILINGDATES) return;

  SetTextColor(hdc, infoPtr->colors[MCSC_TRAILINGTEXT]);

  /* draw prev month */
  MONTHCAL_GetMinDate(infoPtr, &st);
  mask = 1 << (st.wDay-1);
  /* December and January both 31 days long, so no worries if wrapped */
  length = MONTHCAL_MonthLength(infoPtr->calendars[0].month.wMonth - 1,
                                infoPtr->calendars[0].month.wYear);
  index = 0;
  while(st.wDay <= length)
  {
      MONTHCAL_DrawDay(infoPtr, hdc, &st, infoPtr->monthdayState[index] & mask, ps);
      mask <<= 1;
      st.wDay++;
  }

  /* draw next month */
  st = infoPtr->calendars[MONTHCAL_GetCalCount(infoPtr)-1].month;
  st.wDay = 1;
  MONTHCAL_GetNextMonth(&st);
  MONTHCAL_GetMaxDate(infoPtr, &st_max);
  mask = 1;
  index = MONTHCAL_GetMonthRange(infoPtr, GMR_DAYSTATE, 0)-1;
  while(st.wDay <= st_max.wDay)
  {
      MONTHCAL_DrawDay(infoPtr, hdc, &st, infoPtr->monthdayState[index] & mask, ps);
      mask <<= 1;
      st.wDay++;
  }
}

static int get_localized_dayname(const MONTHCAL_INFO *infoPtr, unsigned int day, WCHAR *buff, unsigned int count)
{
  LCTYPE lctype;

  if (infoPtr->dwStyle & MCS_SHORTDAYSOFWEEK)
      lctype = LOCALE_SSHORTESTDAYNAME1 + day;
  else
      lctype = LOCALE_SABBREVDAYNAME1 + day;

  return GetLocaleInfoW(LOCALE_USER_DEFAULT, lctype, buff, count);
}

/* paint a calendar area */
static void MONTHCAL_PaintCalendar(const MONTHCAL_INFO *infoPtr, HDC hdc, const PAINTSTRUCT *ps, INT calIdx)
{
  const SYSTEMTIME *date = &infoPtr->calendars[calIdx].month;
  INT i, j;
  UINT length;
  RECT r, fill_bk_rect;
  SYSTEMTIME st;
  WCHAR buf[80];
  HPEN old_pen;
  int mask;

  /* fill whole days area - from week days area to today note rectangle */
  fill_bk_rect = infoPtr->calendars[calIdx].wdays;
  fill_bk_rect.bottom = infoPtr->calendars[calIdx].days.bottom +
                          (infoPtr->todayrect.bottom - infoPtr->todayrect.top);

  FillRect(hdc, &fill_bk_rect, infoPtr->brushes[BrushMonth]);

  /* draw line under day abbreviations */
  old_pen = SelectObject(hdc, infoPtr->pens[PenText]);
  MoveToEx(hdc, infoPtr->calendars[calIdx].days.left + 3,
                infoPtr->calendars[calIdx].title.bottom + infoPtr->textHeight + 1, NULL);
  LineTo(hdc, infoPtr->calendars[calIdx].days.right - 3,
              infoPtr->calendars[calIdx].title.bottom + infoPtr->textHeight + 1);
  SelectObject(hdc, old_pen);

  infoPtr->calendars[calIdx].wdays.left = infoPtr->calendars[calIdx].days.left =
      infoPtr->calendars[calIdx].weeknums.right;

  /* draw day abbreviations */
  SelectObject(hdc, infoPtr->hFont);
  SetBkColor(hdc, infoPtr->colors[MCSC_MONTHBK]);
  SetTextColor(hdc, infoPtr->colors[MCSC_TITLEBK]);
  /* rectangle to draw a single day abbreviation within */
  r = infoPtr->calendars[calIdx].wdays;
  r.right = r.left + infoPtr->width_increment;

  i = infoPtr->firstDay;
  for(j = 0; j < 7; j++) {
    get_localized_dayname(infoPtr, (i + j + 6) % 7, buf, ARRAY_SIZE(buf));
    DrawTextW(hdc, buf, lstrlenW(buf), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    OffsetRect(&r, infoPtr->width_increment, 0);
  }

  /* draw current month */
  SetTextColor(hdc, infoPtr->colors[MCSC_TEXT]);
  st = *date;
  st.wDay = 1;
  mask = 1;
  length = MONTHCAL_MonthLength(date->wMonth, date->wYear);
  while(st.wDay <= length)
  {
    MONTHCAL_DrawDay(infoPtr, hdc, &st, infoPtr->monthdayState[calIdx+1] & mask, ps);
    mask <<= 1;
    st.wDay++;
  }
}

static void MONTHCAL_Refresh(MONTHCAL_INFO *infoPtr, HDC hdc, const PAINTSTRUCT *ps)
{
  COLORREF old_text_clr, old_bk_clr;
  HFONT old_font;
  INT i;

  old_text_clr = SetTextColor(hdc, comctl32_color.clrWindowText);
  old_bk_clr   = GetBkColor(hdc);
  old_font     = GetCurrentObject(hdc, OBJ_FONT);

  for (i = 0; i < MONTHCAL_GetCalCount(infoPtr); i++)
  {
    RECT *title = &infoPtr->calendars[i].title;
    RECT r;

    /* draw title, redraw all its elements */
    if (IntersectRect(&r, &(ps->rcPaint), title))
        MONTHCAL_PaintTitle(infoPtr, hdc, ps, i);

    /* draw calendar area */
    UnionRect(&r, &infoPtr->calendars[i].wdays, &infoPtr->todayrect);
    if (IntersectRect(&r, &(ps->rcPaint), &r))
        MONTHCAL_PaintCalendar(infoPtr, hdc, ps, i);

    /* week numbers */
    MONTHCAL_PaintWeeknumbers(infoPtr, hdc, ps, i);
  }

  /* partially visible months */
  MONTHCAL_PaintLeadTrailMonths(infoPtr, hdc, ps);

  /* focus and today rectangle */
  MONTHCAL_PaintFocusAndCircle(infoPtr, hdc, ps);

  /* today at the bottom left */
  MONTHCAL_PaintTodayTitle(infoPtr, hdc, ps);

  /* navigation buttons */
  MONTHCAL_PaintButton(infoPtr, hdc, DIRECTION_BACKWARD);
  MONTHCAL_PaintButton(infoPtr, hdc, DIRECTION_FORWARD);

  /* restore context */
  SetBkColor(hdc, old_bk_clr);
  SelectObject(hdc, old_font);
  SetTextColor(hdc, old_text_clr);
}

static LRESULT
MONTHCAL_GetMinReqRect(const MONTHCAL_INFO *infoPtr, RECT *rect)
{
  TRACE("rect %p\n", rect);

  if(!rect) return FALSE;

  *rect = infoPtr->calendars[0].title;
  rect->bottom = infoPtr->calendars[0].days.bottom + infoPtr->todayrect.bottom -
                 infoPtr->todayrect.top;

  AdjustWindowRect(rect, infoPtr->dwStyle, FALSE);

  /* minimal rectangle is zero based */
  OffsetRect(rect, -rect->left, -rect->top);

  TRACE("%s\n", wine_dbgstr_rect(rect));

  return TRUE;
}

static COLORREF
MONTHCAL_GetColor(const MONTHCAL_INFO *infoPtr, UINT index)
{
  TRACE("%p, %d\n", infoPtr, index);

  if (index > MCSC_TRAILINGTEXT) return -1;
  return infoPtr->colors[index];
}

static LRESULT
MONTHCAL_SetColor(MONTHCAL_INFO *infoPtr, UINT index, COLORREF color)
{
  enum CachedBrush type;
  COLORREF prev;

  TRACE("%p, %d: color %08x\n", infoPtr, index, color);

  if (index > MCSC_TRAILINGTEXT) return -1;

  prev = infoPtr->colors[index];
  infoPtr->colors[index] = color;

  /* update cached brush */
  switch (index)
  {
  case MCSC_BACKGROUND:
    type = BrushBackground;
    break;
  case MCSC_TITLEBK:
    type = BrushTitle;
    break;
  case MCSC_MONTHBK:
    type = BrushMonth;
    break;
  default:
    type = BrushLast;
  }

  if (type != BrushLast)
  {
    DeleteObject(infoPtr->brushes[type]);
    infoPtr->brushes[type] = CreateSolidBrush(color);
  }

  /* update cached pen */
  if (index == MCSC_TEXT)
  {
    DeleteObject(infoPtr->pens[PenText]);
    infoPtr->pens[PenText] = CreatePen(PS_SOLID, 1, infoPtr->colors[index]);
  }

  InvalidateRect(infoPtr->hwndSelf, NULL, index == MCSC_BACKGROUND);
  return prev;
}

static LRESULT
MONTHCAL_GetMonthDelta(const MONTHCAL_INFO *infoPtr)
{
  TRACE("\n");

  if(infoPtr->delta)
    return infoPtr->delta;

  return MONTHCAL_GetMonthRange(infoPtr, GMR_VISIBLE, NULL);
}


static LRESULT
MONTHCAL_SetMonthDelta(MONTHCAL_INFO *infoPtr, INT delta)
{
  INT prev = infoPtr->delta;

  TRACE("delta %d\n", delta);

  infoPtr->delta = delta;
  return prev;
}


static inline LRESULT
MONTHCAL_GetFirstDayOfWeek(const MONTHCAL_INFO *infoPtr)
{
  int day;

  /* convert from SYSTEMTIME to locale format */
  day = (infoPtr->firstDay >= 0) ? (infoPtr->firstDay+6)%7 : infoPtr->firstDay;

  return MAKELONG(day, infoPtr->firstDaySet);
}


/* Sets the first day of the week that will appear in the control
 *
 *
 * PARAMETERS:
 *  [I] infoPtr : valid pointer to control data
 *  [I] day : day number to set as new first day (0 == Monday,...,6 == Sunday)
 *
 *
 * RETURN VALUE:
 *  Low word contains previous first day,
 *  high word indicates was first day forced with this message before or is
 *  locale defined (TRUE - was forced, FALSE - wasn't).
 *
 * FIXME: this needs to be implemented properly in MONTHCAL_Refresh()
 * FIXME: we need more error checking here
 */
static LRESULT
MONTHCAL_SetFirstDayOfWeek(MONTHCAL_INFO *infoPtr, INT day)
{
  LRESULT prev = MONTHCAL_GetFirstDayOfWeek(infoPtr);
  int new_day;

  TRACE("%d\n", day);

  if(day == -1)
  {
    WCHAR buf[80];

    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, buf, ARRAY_SIZE(buf));
    TRACE("%s %d\n", debugstr_w(buf), lstrlenW(buf));

    new_day = wcstol(buf, NULL, 10);

    infoPtr->firstDaySet = FALSE;
  }
  else if(day >= 7)
  {
    new_day = 6; /* max first day allowed */
    infoPtr->firstDaySet = TRUE;
  }
  else
  {
    /* Native behaviour for that case is broken: invalid date number >31
       got displayed at (0,0) position, current month starts always from
       (1,0) position. Should be implemented here as well only if there's
       nothing else to do. */
    if (day < -1)
      FIXME("No bug compatibility for day=%d\n", day);

    new_day = day;
    infoPtr->firstDaySet = TRUE;
  }

  /* convert from locale to SYSTEMTIME format */
  infoPtr->firstDay = (new_day >= 0) ? (++new_day) % 7 : new_day;

  InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

  return prev;
}

static LRESULT
MONTHCAL_GetMaxTodayWidth(const MONTHCAL_INFO *infoPtr)
{
  return(infoPtr->todayrect.right - infoPtr->todayrect.left);
}

static LRESULT
MONTHCAL_SetRange(MONTHCAL_INFO *infoPtr, SHORT limits, SYSTEMTIME *range)
{
    FILETIME ft_min, ft_max;

    TRACE("%x %p\n", limits, range);

    if ((limits & GDTR_MIN && !MONTHCAL_ValidateDate(&range[0])) ||
        (limits & GDTR_MAX && !MONTHCAL_ValidateDate(&range[1])))
        return FALSE;

    infoPtr->rangeValid = 0;
    infoPtr->minDate = infoPtr->maxDate = st_null;

    if (limits & GDTR_MIN)
    {
        if (!MONTHCAL_ValidateTime(&range[0]))
            MONTHCAL_CopyTime(&infoPtr->todaysDate, &range[0]);

        infoPtr->minDate = range[0];
        infoPtr->rangeValid |= GDTR_MIN;
    }
    if (limits & GDTR_MAX)
    {
        if (!MONTHCAL_ValidateTime(&range[1]))
            MONTHCAL_CopyTime(&infoPtr->todaysDate, &range[1]);

        infoPtr->maxDate = range[1];
        infoPtr->rangeValid |= GDTR_MAX;
    }

    /* Only one limit set - we are done */
    if ((infoPtr->rangeValid & (GDTR_MIN | GDTR_MAX)) != (GDTR_MIN | GDTR_MAX))
        return TRUE;

    SystemTimeToFileTime(&infoPtr->maxDate, &ft_max);
    SystemTimeToFileTime(&infoPtr->minDate, &ft_min);

    if (CompareFileTime(&ft_min, &ft_max) >= 0)
    {
        if ((limits & (GDTR_MIN | GDTR_MAX)) == (GDTR_MIN | GDTR_MAX))
        {
            /* Native swaps limits only when both limits are being set. */
            SYSTEMTIME st_tmp = infoPtr->minDate;
            infoPtr->minDate  = infoPtr->maxDate;
            infoPtr->maxDate  = st_tmp;
        }
        else
        {
            /* reset the other limit */
            if (limits & GDTR_MIN) infoPtr->maxDate = st_null;
            if (limits & GDTR_MAX) infoPtr->minDate = st_null;
            infoPtr->rangeValid &= limits & GDTR_MIN ? ~GDTR_MAX : ~GDTR_MIN;
        }
    }

    return TRUE;
}


static LRESULT
MONTHCAL_GetRange(const MONTHCAL_INFO *infoPtr, SYSTEMTIME *range)
{
  TRACE("%p\n", range);

  if (!range) return 0;

  range[1] = infoPtr->maxDate;
  range[0] = infoPtr->minDate;

  return infoPtr->rangeValid;
}


static LRESULT
MONTHCAL_SetDayState(const MONTHCAL_INFO *infoPtr, INT months, MONTHDAYSTATE *states)
{
  TRACE("%p %d %p\n", infoPtr, months, states);

  if (!(infoPtr->dwStyle & MCS_DAYSTATE)) return 0;
  if (months != MONTHCAL_GetMonthRange(infoPtr, GMR_DAYSTATE, 0)) return 0;

  memcpy(infoPtr->monthdayState, states, months*sizeof(MONTHDAYSTATE));

  return 1;
}

static LRESULT
MONTHCAL_GetCurSel(const MONTHCAL_INFO *infoPtr, SYSTEMTIME *curSel)
{
  TRACE("%p\n", curSel);
  if(!curSel) return FALSE;
  if(infoPtr->dwStyle & MCS_MULTISELECT) return FALSE;

  *curSel = infoPtr->minSel;
  TRACE("%d/%d/%d\n", curSel->wYear, curSel->wMonth, curSel->wDay);
  return TRUE;
}

static LRESULT
MONTHCAL_SetCurSel(MONTHCAL_INFO *infoPtr, SYSTEMTIME *curSel)
{
  SYSTEMTIME prev = infoPtr->minSel, selection;
  INT diff;
  WORD day;

  TRACE("%p\n", curSel);
  if(!curSel) return FALSE;
  if(infoPtr->dwStyle & MCS_MULTISELECT) return FALSE;

  if(!MONTHCAL_ValidateDate(curSel)) return FALSE;
  /* exit earlier if selection equals current */
  if (MONTHCAL_IsDateEqual(&infoPtr->minSel, curSel)) return TRUE;

  selection = *curSel;
  selection.wHour = selection.wMinute = selection.wSecond = selection.wMilliseconds = 0;
  MONTHCAL_CalculateDayOfWeek(&selection, TRUE);

  if(!MONTHCAL_IsDateInValidRange(infoPtr, &selection, FALSE)) return FALSE;

  /* scroll calendars only if we have to */
  diff = MONTHCAL_MonthDiff(&infoPtr->calendars[MONTHCAL_GetCalCount(infoPtr)-1].month, curSel);
  if (diff <= 0)
  {
    diff = MONTHCAL_MonthDiff(&infoPtr->calendars[0].month, curSel);
    if (diff > 0) diff = 0;
  }

  if (diff != 0)
  {
    INT i;

    for (i = 0; i < MONTHCAL_GetCalCount(infoPtr); i++)
      MONTHCAL_GetMonth(&infoPtr->calendars[i].month, diff);
  }

  /* we need to store time part as it is */
  selection = *curSel;
  MONTHCAL_CalculateDayOfWeek(&selection, TRUE);
  infoPtr->minSel = infoPtr->maxSel = selection;

  /* if selection is still in current month, reduce rectangle */
  day = prev.wDay;
  prev.wDay = curSel->wDay;
  if (MONTHCAL_IsDateEqual(&prev, curSel))
  {
    RECT r_prev, r_new;

    prev.wDay = day;
    MONTHCAL_GetDayRect(infoPtr, &prev, &r_prev, -1);
    MONTHCAL_GetDayRect(infoPtr, curSel, &r_new, -1);

    InvalidateRect(infoPtr->hwndSelf, &r_prev, FALSE);
    InvalidateRect(infoPtr->hwndSelf, &r_new,  FALSE);
  }
  else
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

  return TRUE;
}


static LRESULT
MONTHCAL_GetMaxSelCount(const MONTHCAL_INFO *infoPtr)
{
  return infoPtr->maxSelCount;
}


static LRESULT
MONTHCAL_SetMaxSelCount(MONTHCAL_INFO *infoPtr, INT max)
{
  TRACE("%d\n", max);

  if(!(infoPtr->dwStyle & MCS_MULTISELECT)) return FALSE;
  if(max <= 0) return FALSE;

  infoPtr->maxSelCount = max;

  return TRUE;
}


static LRESULT
MONTHCAL_GetSelRange(const MONTHCAL_INFO *infoPtr, SYSTEMTIME *range)
{
  TRACE("%p\n", range);

  if(!range) return FALSE;

  if(infoPtr->dwStyle & MCS_MULTISELECT)
  {
    range[1] = infoPtr->maxSel;
    range[0] = infoPtr->minSel;
    TRACE("[min,max]=[%d %d]\n", infoPtr->minSel.wDay, infoPtr->maxSel.wDay);
    return TRUE;
  }

  return FALSE;
}


static LRESULT
MONTHCAL_SetSelRange(MONTHCAL_INFO *infoPtr, SYSTEMTIME *range)
{
  SYSTEMTIME old_range[2];
  INT diff;

  TRACE("%p\n", range);

  if(!range || !(infoPtr->dwStyle & MCS_MULTISELECT)) return FALSE;

  /* adjust timestamps */
  if(!MONTHCAL_ValidateTime(&range[0])) MONTHCAL_CopyTime(&infoPtr->todaysDate, &range[0]);
  if(!MONTHCAL_ValidateTime(&range[1])) MONTHCAL_CopyTime(&infoPtr->todaysDate, &range[1]);

  /* maximum range exceeded */
  if(!MONTHCAL_IsSelRangeValid(infoPtr, &range[0], &range[1], NULL)) return FALSE;

  old_range[0] = infoPtr->minSel;
  old_range[1] = infoPtr->maxSel;

  /* swap if min > max */
  if(MONTHCAL_CompareSystemTime(&range[0], &range[1]) <= 0)
  {
    infoPtr->minSel = range[0];
    infoPtr->maxSel = range[1];
  }
  else
  {
    infoPtr->minSel = range[1];
    infoPtr->maxSel = range[0];
  }

  diff = MONTHCAL_MonthDiff(&infoPtr->calendars[MONTHCAL_GetCalCount(infoPtr)-1].month, &infoPtr->maxSel);
  if (diff < 0)
  {
    diff = MONTHCAL_MonthDiff(&infoPtr->calendars[0].month, &infoPtr->maxSel);
    if (diff > 0) diff = 0;
  }

  if (diff != 0)
  {
    INT i;

    for (i = 0; i < MONTHCAL_GetCalCount(infoPtr); i++)
      MONTHCAL_GetMonth(&infoPtr->calendars[i].month, diff);
  }

  /* update day of week */
  MONTHCAL_CalculateDayOfWeek(&infoPtr->minSel, TRUE);
  MONTHCAL_CalculateDayOfWeek(&infoPtr->maxSel, TRUE);

  /* redraw if bounds changed */
  /* FIXME: no actual need to redraw everything */
  if(!MONTHCAL_IsDateEqual(&old_range[0], &range[0]) ||
     !MONTHCAL_IsDateEqual(&old_range[1], &range[1]))
  {
     InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
  }

  TRACE("[min,max]=[%d %d]\n", infoPtr->minSel.wDay, infoPtr->maxSel.wDay);
  return TRUE;
}


static LRESULT
MONTHCAL_GetToday(const MONTHCAL_INFO *infoPtr, SYSTEMTIME *today)
{
  TRACE("%p\n", today);

  if(!today) return FALSE;
  *today = infoPtr->todaysDate;
  return TRUE;
}

/* Internal helper for MCM_SETTODAY handler and auto update timer handler
 *
 * RETURN VALUE
 *
 *  TRUE  - today date changed
 *  FALSE - today date isn't changed
 */
static BOOL
MONTHCAL_UpdateToday(MONTHCAL_INFO *infoPtr, const SYSTEMTIME *today)
{
    RECT rect;

    if (MONTHCAL_IsDateEqual(today, &infoPtr->todaysDate))
        return FALSE;

    /* Invalidate old and new today day rectangle, and today label. */
    if (MONTHCAL_GetDayRect(infoPtr, &infoPtr->todaysDate, &rect, -1))
        InvalidateRect(infoPtr->hwndSelf, &rect, FALSE);

    if (MONTHCAL_GetDayRect(infoPtr, today, &rect, -1))
        InvalidateRect(infoPtr->hwndSelf, &rect, FALSE);

    infoPtr->todaysDate = *today;

    InvalidateRect(infoPtr->hwndSelf, &infoPtr->todayrect, FALSE);
    return TRUE;
}

/* MCM_SETTODAT handler */
static LRESULT
MONTHCAL_SetToday(MONTHCAL_INFO *infoPtr, const SYSTEMTIME *today)
{
  TRACE("%p\n", today);

  if (today)
  {
    /* remember if date was set successfully */
    if (MONTHCAL_UpdateToday(infoPtr, today)) infoPtr->todaySet = TRUE;
  }

  return 0;
}

/* returns calendar index containing specified point, or -1 if it's background */
static INT MONTHCAL_GetCalendarFromPoint(const MONTHCAL_INFO *infoPtr, const POINT *pt)
{
  RECT r;
  INT i;

  for (i = 0; i < MONTHCAL_GetCalCount(infoPtr); i++)
  {
     /* whole bounding rectangle allows some optimization to compute */
     r.left   = infoPtr->calendars[i].title.left;
     r.top    = infoPtr->calendars[i].title.top;
     r.bottom = infoPtr->calendars[i].days.bottom;
     r.right  = infoPtr->calendars[i].days.right;

     if (PtInRect(&r, *pt)) return i;
  }

  return -1;
}

static inline UINT fill_hittest_info(const MCHITTESTINFO *src, MCHITTESTINFO *dest)
{
  dest->uHit = src->uHit;
  dest->st = src->st;

  if (dest->cbSize == sizeof(MCHITTESTINFO))
    memcpy(&dest->rc, &src->rc, sizeof(MCHITTESTINFO) - MCHITTESTINFO_V1_SIZE);

  return src->uHit;
}

static LRESULT
MONTHCAL_HitTest(const MONTHCAL_INFO *infoPtr, MCHITTESTINFO *lpht)
{
  MCHITTESTINFO htinfo;
  SYSTEMTIME *ht_month;
  INT day, calIdx;

  if(!lpht || lpht->cbSize < MCHITTESTINFO_V1_SIZE) return -1;

  htinfo.st = st_null;

  /* we should preserve passed fields if hit area doesn't need them */
  if (lpht->cbSize == sizeof(MCHITTESTINFO))
    memcpy(&htinfo.rc, &lpht->rc, sizeof(MCHITTESTINFO) - MCHITTESTINFO_V1_SIZE);

  /* guess in what calendar we are */
  calIdx = MONTHCAL_GetCalendarFromPoint(infoPtr, &lpht->pt);
  if (calIdx == -1)
  {
    if (PtInRect(&infoPtr->todayrect, lpht->pt))
    {
      htinfo.uHit = MCHT_TODAYLINK;
      htinfo.rc = infoPtr->todayrect;
    }
    else
      /* outside of calendar area? What's left must be background :-) */
      htinfo.uHit = MCHT_CALENDARBK;

    return fill_hittest_info(&htinfo, lpht);
  }

  /* are we in the header? */
  if (PtInRect(&infoPtr->calendars[calIdx].title, lpht->pt)) {
    /* FIXME: buttons hittesting could be optimized cause maximum
              two calendars have buttons */
    if (calIdx == 0 && PtInRect(&infoPtr->titlebtnprev, lpht->pt))
    {
      htinfo.uHit = MCHT_TITLEBTNPREV;
      htinfo.rc = infoPtr->titlebtnprev;
    }
    else if (PtInRect(&infoPtr->titlebtnnext, lpht->pt))
    {
      htinfo.uHit = MCHT_TITLEBTNNEXT;
      htinfo.rc = infoPtr->titlebtnnext;
    }
    else if (PtInRect(&infoPtr->calendars[calIdx].titlemonth, lpht->pt))
    {
      htinfo.uHit = MCHT_TITLEMONTH;
      htinfo.rc = infoPtr->calendars[calIdx].titlemonth;
      htinfo.iOffset = calIdx;
    }
    else if (PtInRect(&infoPtr->calendars[calIdx].titleyear, lpht->pt))
    {
      htinfo.uHit = MCHT_TITLEYEAR;
      htinfo.rc = infoPtr->calendars[calIdx].titleyear;
      htinfo.iOffset = calIdx;
    }
    else
    {
      htinfo.uHit = MCHT_TITLE;
      htinfo.rc = infoPtr->calendars[calIdx].title;
      htinfo.iOffset = calIdx;
    }

    return fill_hittest_info(&htinfo, lpht);
  }

  ht_month = &infoPtr->calendars[calIdx].month;
  /* days area (including week days and week numbers) */
  day = MONTHCAL_GetDayFromPos(infoPtr, lpht->pt, calIdx);
  if (PtInRect(&infoPtr->calendars[calIdx].wdays, lpht->pt))
  {
    htinfo.uHit = MCHT_CALENDARDAY;
    htinfo.iOffset = calIdx;
    htinfo.st.wYear  = ht_month->wYear;
    htinfo.st.wMonth = (day < 1) ? ht_month->wMonth -1 : ht_month->wMonth;
    htinfo.st.wDay   = (day < 1) ?
      MONTHCAL_MonthLength(ht_month->wMonth-1, ht_month->wYear) - day : day;

    MONTHCAL_GetDayPos(infoPtr, &htinfo.st, &htinfo.iCol, &htinfo.iRow, calIdx);
  }
  else if(PtInRect(&infoPtr->calendars[calIdx].weeknums, lpht->pt))
  {
    htinfo.uHit = MCHT_CALENDARWEEKNUM;
    htinfo.st.wYear = ht_month->wYear;
    htinfo.iOffset = calIdx;

    if (day < 1)
    {
      htinfo.st.wMonth = ht_month->wMonth - 1;
      htinfo.st.wDay = MONTHCAL_MonthLength(ht_month->wMonth-1, ht_month->wYear) - day;
    }
    else if (day > MONTHCAL_MonthLength(ht_month->wMonth, ht_month->wYear))
    {
      htinfo.st.wMonth = ht_month->wMonth + 1;
      htinfo.st.wDay = day - MONTHCAL_MonthLength(ht_month->wMonth, ht_month->wYear);
    }
    else
    {
      htinfo.st.wMonth = ht_month->wMonth;
      htinfo.st.wDay = day;
    }
  }
  else if(PtInRect(&infoPtr->calendars[calIdx].days, lpht->pt))
  {
      htinfo.iOffset = calIdx;
      htinfo.st.wDay = ht_month->wDay;
      htinfo.st.wYear  = ht_month->wYear;
      htinfo.st.wMonth = ht_month->wMonth;
      /* previous month only valid for first calendar */
      if (day < 1 && calIdx == 0)
      {
	  htinfo.uHit = MCHT_CALENDARDATEPREV;
	  MONTHCAL_GetPrevMonth(&htinfo.st);
	  htinfo.st.wDay = MONTHCAL_MonthLength(htinfo.st.wMonth, htinfo.st.wYear) + day;
      }
      /* next month only valid for last calendar */
      else if (day > MONTHCAL_MonthLength(ht_month->wMonth, ht_month->wYear) &&
               calIdx == MONTHCAL_GetCalCount(infoPtr)-1)
      {
	  htinfo.uHit = MCHT_CALENDARDATENEXT;
	  MONTHCAL_GetNextMonth(&htinfo.st);
	  htinfo.st.wDay = day - MONTHCAL_MonthLength(ht_month->wMonth, ht_month->wYear);
      }
      /* multiple calendars case - blank areas for previous/next month */
      else if (day < 1 || day > MONTHCAL_MonthLength(ht_month->wMonth, ht_month->wYear))
      {
          htinfo.uHit = MCHT_CALENDARBK;
      }
      else
      {
	htinfo.uHit = MCHT_CALENDARDATE;
	htinfo.st.wDay = day;
      }

      MONTHCAL_GetDayPos(infoPtr, &htinfo.st, &htinfo.iCol, &htinfo.iRow, calIdx);
      MONTHCAL_GetDayRectI(infoPtr, &htinfo.rc, htinfo.iCol, htinfo.iRow, calIdx);
      /* always update day of week */
      MONTHCAL_CalculateDayOfWeek(&htinfo.st, TRUE);
  }

  return fill_hittest_info(&htinfo, lpht);
}

/* MCN_GETDAYSTATE notification helper */
static void MONTHCAL_NotifyDayState(MONTHCAL_INFO *infoPtr)
{
  MONTHDAYSTATE *state;
  NMDAYSTATE nmds;

  if (!(infoPtr->dwStyle & MCS_DAYSTATE)) return;

  nmds.nmhdr.hwndFrom = infoPtr->hwndSelf;
  nmds.nmhdr.idFrom   = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
  nmds.nmhdr.code     = MCN_GETDAYSTATE;
  nmds.cDayState      = MONTHCAL_GetMonthRange(infoPtr, GMR_DAYSTATE, 0);
  nmds.prgDayState    = state = heap_alloc_zero(nmds.cDayState * sizeof(MONTHDAYSTATE));

  MONTHCAL_GetMinDate(infoPtr, &nmds.stStart);
  nmds.stStart.wDay = 1;

  SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, nmds.nmhdr.idFrom, (LPARAM)&nmds);
  memcpy(infoPtr->monthdayState, nmds.prgDayState,
      MONTHCAL_GetMonthRange(infoPtr, GMR_DAYSTATE, 0)*sizeof(MONTHDAYSTATE));

  heap_free(state);
}

/* no valid range check performed */
static void MONTHCAL_Scroll(MONTHCAL_INFO *infoPtr, INT delta, BOOL keep_selection)
{
  INT i, selIdx = -1;

  for(i = 0; i < MONTHCAL_GetCalCount(infoPtr); i++)
  {
    /* save selection position to shift it later */
    if (selIdx == -1 && MONTHCAL_CompareMonths(&infoPtr->minSel, &infoPtr->calendars[i].month) == 0)
      selIdx = i;

    MONTHCAL_GetMonth(&infoPtr->calendars[i].month, delta);
  }

  if (keep_selection)
    return;

  /* selection is always shifted to first calendar */
  if (infoPtr->dwStyle & MCS_MULTISELECT)
  {
    SYSTEMTIME range[2];

    MONTHCAL_GetSelRange(infoPtr, range);
    MONTHCAL_GetMonth(&range[0], delta - selIdx);
    MONTHCAL_GetMonth(&range[1], delta - selIdx);
    MONTHCAL_SetSelRange(infoPtr, range);
  }
  else
  {
    SYSTEMTIME st = infoPtr->minSel;

    MONTHCAL_GetMonth(&st, delta - selIdx);
    MONTHCAL_SetCurSel(infoPtr, &st);
  }
}

static void MONTHCAL_GoToMonth(MONTHCAL_INFO *infoPtr, enum nav_direction direction)
{
  INT delta = infoPtr->delta ? infoPtr->delta : MONTHCAL_GetCalCount(infoPtr);
  BOOL keep_selection;
  SYSTEMTIME st;

  TRACE("%s\n", direction == DIRECTION_BACKWARD ? "back" : "fwd");

  /* check if change allowed by range set */
  if(direction == DIRECTION_BACKWARD)
  {
    st = infoPtr->calendars[0].month;
    MONTHCAL_GetMonth(&st, -delta);
  }
  else
  {
    st = infoPtr->calendars[MONTHCAL_GetCalCount(infoPtr)-1].month;
    MONTHCAL_GetMonth(&st, delta);
  }

  if(!MONTHCAL_IsDateInValidRange(infoPtr, &st, FALSE)) return;

  keep_selection = infoPtr->dwStyle & MCS_NOSELCHANGEONNAV;
  MONTHCAL_Scroll(infoPtr, direction == DIRECTION_BACKWARD ? -delta : delta, keep_selection);
  MONTHCAL_NotifyDayState(infoPtr);
  if (!keep_selection)
    MONTHCAL_NotifySelectionChange(infoPtr);
}

static LRESULT
MONTHCAL_RButtonUp(MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  HMENU hMenu;
  POINT menupoint;
  WCHAR buf[32];

  hMenu = CreatePopupMenu();
  LoadStringW(COMCTL32_hModule, IDM_GOTODAY, buf, ARRAY_SIZE(buf));
  AppendMenuW(hMenu, MF_STRING|MF_ENABLED, 1, buf);
  menupoint.x = (short)LOWORD(lParam);
  menupoint.y = (short)HIWORD(lParam);
  ClientToScreen(infoPtr->hwndSelf, &menupoint);
  if( TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
		     menupoint.x, menupoint.y, 0, infoPtr->hwndSelf, NULL))
  {
      if (infoPtr->dwStyle & MCS_MULTISELECT)
      {
          SYSTEMTIME range[2];

          range[0] = range[1] = infoPtr->todaysDate;
          MONTHCAL_SetSelRange(infoPtr, range);
      }
      else
          MONTHCAL_SetCurSel(infoPtr, &infoPtr->todaysDate);

      MONTHCAL_NotifySelectionChange(infoPtr);
      MONTHCAL_NotifySelect(infoPtr);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Subclassed edit control windproc function
 *
 * PARAMETER(S):
 * [I] hwnd : the edit window handle
 * [I] uMsg : the message that is to be processed
 * [I] wParam : first message parameter
 * [I] lParam : second message parameter
 *
 */
static LRESULT CALLBACK EditWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MONTHCAL_INFO *infoPtr = (MONTHCAL_INFO *)GetWindowLongPtrW(GetParent(hwnd), 0);

    TRACE("(hwnd=%p, uMsg=%x, wParam=%lx, lParam=%lx)\n",
	  hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
	case WM_GETDLGCODE:
	  return DLGC_WANTARROWS | DLGC_WANTALLKEYS;

	case WM_DESTROY:
	{
	    WNDPROC editProc = infoPtr->EditWndProc;
	    infoPtr->EditWndProc = NULL;
	    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (DWORD_PTR)editProc);
	    return CallWindowProcW(editProc, hwnd, uMsg, wParam, lParam);
	}

	case WM_KILLFOCUS:
	    break;

	case WM_KEYDOWN:
	    if ((VK_ESCAPE == (INT)wParam) || (VK_RETURN == (INT)wParam))
		break;

	default:
	    return CallWindowProcW(infoPtr->EditWndProc, hwnd, uMsg, wParam, lParam);
    }

    SendMessageW(infoPtr->hWndYearUpDown, WM_CLOSE, 0, 0);
    SendMessageW(hwnd, WM_CLOSE, 0, 0);
    return 0;
}

/* creates updown control and edit box */
static void MONTHCAL_EditYear(MONTHCAL_INFO *infoPtr, INT calIdx)
{
    RECT *rc = &infoPtr->calendars[calIdx].titleyear;
    RECT *title = &infoPtr->calendars[calIdx].title;

    infoPtr->hWndYearEdit =
	CreateWindowExW(0, WC_EDITW, 0, WS_VISIBLE | WS_CHILD | ES_READONLY,
			rc->left + 3, (title->bottom + title->top - infoPtr->textHeight) / 2,
			rc->right - rc->left + 4,
			infoPtr->textHeight, infoPtr->hwndSelf,
			NULL, NULL, NULL);

    SendMessageW(infoPtr->hWndYearEdit, WM_SETFONT, (WPARAM)infoPtr->hBoldFont, TRUE);

    infoPtr->hWndYearUpDown =
	CreateWindowExW(0, UPDOWN_CLASSW, 0,
			WS_VISIBLE | WS_CHILD | UDS_SETBUDDYINT | UDS_NOTHOUSANDS | UDS_ARROWKEYS,
			rc->right + 7, (title->bottom + title->top - infoPtr->textHeight) / 2,
			18, infoPtr->textHeight, infoPtr->hwndSelf,
			NULL, NULL, NULL);

    /* attach edit box */
    SendMessageW(infoPtr->hWndYearUpDown, UDM_SETRANGE, 0,
                 MAKELONG(max_allowed_date.wYear, min_allowed_date.wYear));
    SendMessageW(infoPtr->hWndYearUpDown, UDM_SETBUDDY, (WPARAM)infoPtr->hWndYearEdit, 0);
    SendMessageW(infoPtr->hWndYearUpDown, UDM_SETPOS, 0, infoPtr->calendars[calIdx].month.wYear);

    /* subclass edit box */
    infoPtr->EditWndProc = (WNDPROC)SetWindowLongPtrW(infoPtr->hWndYearEdit,
                                  GWLP_WNDPROC, (DWORD_PTR)EditWndProc);

    SetFocus(infoPtr->hWndYearEdit);
}

static LRESULT
MONTHCAL_LButtonDown(MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  MCHITTESTINFO ht;
  DWORD hit;

  /* Actually we don't need input focus for calendar, this is used to kill
     year updown and its buddy edit box */
  if (IsWindow(infoPtr->hWndYearUpDown))
  {
      SetFocus(infoPtr->hwndSelf);
      return 0;
  }

  SetCapture(infoPtr->hwndSelf);

  ht.cbSize = sizeof(MCHITTESTINFO);
  ht.pt.x = (short)LOWORD(lParam);
  ht.pt.y = (short)HIWORD(lParam);

  hit = MONTHCAL_HitTest(infoPtr, &ht);

  TRACE("%x at %s\n", hit, wine_dbgstr_point(&ht.pt));

  switch(hit)
  {
  case MCHT_TITLEBTNNEXT:
    MONTHCAL_GoToMonth(infoPtr, DIRECTION_FORWARD);
    infoPtr->status = MC_NEXTPRESSED;
    SetTimer(infoPtr->hwndSelf, MC_PREVNEXTMONTHTIMER, MC_PREVNEXTMONTHDELAY, 0);
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
    return 0;

  case MCHT_TITLEBTNPREV:
    MONTHCAL_GoToMonth(infoPtr, DIRECTION_BACKWARD);
    infoPtr->status = MC_PREVPRESSED;
    SetTimer(infoPtr->hwndSelf, MC_PREVNEXTMONTHTIMER, MC_PREVNEXTMONTHDELAY, 0);
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
    return 0;

  case MCHT_TITLEMONTH:
  {
    HMENU hMenu = CreatePopupMenu();
    WCHAR buf[32];
    POINT menupoint;
    INT i;

    for (i = 0; i < 12; i++)
    {
        GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHNAME1+i, buf, ARRAY_SIZE(buf));
        AppendMenuW(hMenu, MF_STRING|MF_ENABLED, i + 1, buf);
    }
    menupoint.x = ht.pt.x;
    menupoint.y = ht.pt.y;
    ClientToScreen(infoPtr->hwndSelf, &menupoint);
    i = TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_RETURNCMD,
		       menupoint.x, menupoint.y, 0, infoPtr->hwndSelf, NULL);

    if ((i > 0) && (i < 13) && infoPtr->calendars[ht.iOffset].month.wMonth != i)
    {
        INT delta = i - infoPtr->calendars[ht.iOffset].month.wMonth;
        SYSTEMTIME st;

        /* check if change allowed by range set */
        st = delta < 0 ? infoPtr->calendars[0].month :
                         infoPtr->calendars[MONTHCAL_GetCalCount(infoPtr)-1].month;
        MONTHCAL_GetMonth(&st, delta);

        if (MONTHCAL_IsDateInValidRange(infoPtr, &st, FALSE))
        {
            MONTHCAL_Scroll(infoPtr, delta, FALSE);
            MONTHCAL_NotifyDayState(infoPtr);
            MONTHCAL_NotifySelectionChange(infoPtr);
            InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
        }
    }
    return 0;
  }
  case MCHT_TITLEYEAR:
  {
    MONTHCAL_EditYear(infoPtr, ht.iOffset);
    return 0;
  }
  case MCHT_TODAYLINK:
  {
    if (infoPtr->dwStyle & MCS_MULTISELECT)
    {
        SYSTEMTIME range[2];

        range[0] = range[1] = infoPtr->todaysDate;
        MONTHCAL_SetSelRange(infoPtr, range);
    }
    else
        MONTHCAL_SetCurSel(infoPtr, &infoPtr->todaysDate);

    MONTHCAL_NotifySelectionChange(infoPtr);
    MONTHCAL_NotifySelect(infoPtr);
    return 0;
  }
  case MCHT_CALENDARDATENEXT:
  case MCHT_CALENDARDATEPREV:
  case MCHT_CALENDARDATE:
  {
    SYSTEMTIME st[2];

    MONTHCAL_CopyDate(&ht.st, &infoPtr->firstSel);

    st[0] = st[1] = ht.st;
    /* clear selection range */
    MONTHCAL_SetSelRange(infoPtr, st);

    infoPtr->status = MC_SEL_LBUTDOWN;
    MONTHCAL_SetDayFocus(infoPtr, &ht.st);
    return 0;
  }
  }

  return 1;
}


static LRESULT
MONTHCAL_LButtonUp(MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  NMHDR nmhdr;
  MCHITTESTINFO ht;
  DWORD hit;

  TRACE("\n");

  if(infoPtr->status & (MC_PREVPRESSED | MC_NEXTPRESSED)) {
    RECT *r;

    KillTimer(infoPtr->hwndSelf, MC_PREVNEXTMONTHTIMER);
    r = infoPtr->status & MC_PREVPRESSED ? &infoPtr->titlebtnprev : &infoPtr->titlebtnnext;
    infoPtr->status &= ~(MC_PREVPRESSED | MC_NEXTPRESSED);

    InvalidateRect(infoPtr->hwndSelf, r, FALSE);
  }

  ReleaseCapture();

  /* always send NM_RELEASEDCAPTURE notification */
  nmhdr.hwndFrom = infoPtr->hwndSelf;
  nmhdr.idFrom   = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
  nmhdr.code     = NM_RELEASEDCAPTURE;
  TRACE("Sent notification from %p to %p\n", infoPtr->hwndSelf, infoPtr->hwndNotify);

  SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);

  if(!(infoPtr->status & MC_SEL_LBUTDOWN)) return 0;

  ht.cbSize = sizeof(MCHITTESTINFO);
  ht.pt.x = (short)LOWORD(lParam);
  ht.pt.y = (short)HIWORD(lParam);
  hit = MONTHCAL_HitTest(infoPtr, &ht);

  infoPtr->status = MC_SEL_LBUTUP;
  MONTHCAL_SetDayFocus(infoPtr, NULL);

  if((hit & MCHT_CALENDARDATE) == MCHT_CALENDARDATE)
  {
    SYSTEMTIME sel = infoPtr->minSel;

    /* will be invalidated here */
    MONTHCAL_SetCurSel(infoPtr, &ht.st);

    /* send MCN_SELCHANGE only if new date selected */
    if (!MONTHCAL_IsDateEqual(&sel, &ht.st))
        MONTHCAL_NotifySelectionChange(infoPtr);

    MONTHCAL_NotifySelect(infoPtr);
  }

  return 0;
}


static LRESULT
MONTHCAL_Timer(MONTHCAL_INFO *infoPtr, WPARAM id)
{
  TRACE("%ld\n", id);

  switch(id) {
  case MC_PREVNEXTMONTHTIMER:
    if(infoPtr->status & MC_NEXTPRESSED) MONTHCAL_GoToMonth(infoPtr, DIRECTION_FORWARD);
    if(infoPtr->status & MC_PREVPRESSED) MONTHCAL_GoToMonth(infoPtr, DIRECTION_BACKWARD);
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
    break;
  case MC_TODAYUPDATETIMER:
  {
    SYSTEMTIME st;

    if(infoPtr->todaySet) return 0;

    GetLocalTime(&st);
    MONTHCAL_UpdateToday(infoPtr, &st);

    /* notification sent anyway */
    MONTHCAL_NotifySelectionChange(infoPtr);

    return 0;
  }
  default:
    ERR("got unknown timer %ld\n", id);
    break;
  }

  return 0;
}


static LRESULT
MONTHCAL_MouseMove(MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  MCHITTESTINFO ht;
  SYSTEMTIME st_ht;
  INT hit;
  RECT r;

  if(!(infoPtr->status & MC_SEL_LBUTDOWN)) return 0;

  ht.cbSize = sizeof(MCHITTESTINFO);
  ht.pt.x = (short)LOWORD(lParam);
  ht.pt.y = (short)HIWORD(lParam);
  ht.iOffset = -1;

  hit = MONTHCAL_HitTest(infoPtr, &ht);

  /* not on the calendar date numbers? bail out */
  TRACE("hit:%x\n",hit);
  if((hit & MCHT_CALENDARDATE) != MCHT_CALENDARDATE)
  {
    MONTHCAL_SetDayFocus(infoPtr, NULL);
    return 0;
  }

  st_ht = ht.st;

  /* if pointer is over focused day still there's nothing to do */
  if(!MONTHCAL_SetDayFocus(infoPtr, &ht.st)) return 0;

  MONTHCAL_GetDayRect(infoPtr, &ht.st, &r, ht.iOffset);

  if(infoPtr->dwStyle & MCS_MULTISELECT) {
    SYSTEMTIME st[2];

    MONTHCAL_GetSelRange(infoPtr, st);

    /* If we're still at the first selected date and range is empty, return.
       If range isn't empty we should change range to a single firstSel */
    if(MONTHCAL_IsDateEqual(&infoPtr->firstSel, &st_ht) &&
       MONTHCAL_IsDateEqual(&st[0], &st[1])) goto done;

    MONTHCAL_IsSelRangeValid(infoPtr, &st_ht, &infoPtr->firstSel, &st_ht);

    st[0] = infoPtr->firstSel;
    /* we should overwrite timestamp here */
    MONTHCAL_CopyDate(&st_ht, &st[1]);

    /* bounds will be swapped here if needed */
    MONTHCAL_SetSelRange(infoPtr, st);

    return 0;
  }

done:

  /* FIXME: this should specify a rectangle containing only the days that changed
     using InvalidateRect */
  InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

  return 0;
}


static LRESULT
MONTHCAL_Paint(MONTHCAL_INFO *infoPtr, HDC hdc_paint)
{
  HDC hdc;
  PAINTSTRUCT ps;

  if (hdc_paint)
  {
    GetClientRect(infoPtr->hwndSelf, &ps.rcPaint);
    hdc = hdc_paint;
  }
  else
    hdc = BeginPaint(infoPtr->hwndSelf, &ps);

  MONTHCAL_Refresh(infoPtr, hdc, &ps);
  if (!hdc_paint) EndPaint(infoPtr->hwndSelf, &ps);
  return 0;
}

static LRESULT
MONTHCAL_EraseBkgnd(const MONTHCAL_INFO *infoPtr, HDC hdc)
{
  RECT rc;

  if (!GetClipBox(hdc, &rc)) return FALSE;

  FillRect(hdc, &rc, infoPtr->brushes[BrushBackground]);

  return TRUE;
}

static LRESULT
MONTHCAL_PrintClient(MONTHCAL_INFO *infoPtr, HDC hdc, DWORD options)
{
  FIXME("Partial Stub: (hdc=%p options=0x%08x)\n", hdc, options);

  if ((options & PRF_CHECKVISIBLE) && !IsWindowVisible(infoPtr->hwndSelf))
      return 0;

  if (options & PRF_ERASEBKGND)
      MONTHCAL_EraseBkgnd(infoPtr, hdc);

  if (options & PRF_CLIENT)
      MONTHCAL_Paint(infoPtr, hdc);

  return 0;
}

static LRESULT
MONTHCAL_SetFocus(const MONTHCAL_INFO *infoPtr)
{
  TRACE("\n");

  InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

  return 0;
}

/* sets the size information */
static void MONTHCAL_UpdateSize(MONTHCAL_INFO *infoPtr)
{
  static const WCHAR O0W[] = { '0','0',0 };
  RECT *title=&infoPtr->calendars[0].title;
  RECT *prev=&infoPtr->titlebtnprev;
  RECT *next=&infoPtr->titlebtnnext;
  RECT *titlemonth=&infoPtr->calendars[0].titlemonth;
  RECT *titleyear=&infoPtr->calendars[0].titleyear;
  RECT *wdays=&infoPtr->calendars[0].wdays;
  RECT *weeknumrect=&infoPtr->calendars[0].weeknums;
  RECT *days=&infoPtr->calendars[0].days;
  RECT *todayrect=&infoPtr->todayrect;

  INT xdiv, dx, dy, i, j, x, y, c_dx, c_dy;
  WCHAR buff[80];
  TEXTMETRICW tm;
  INT day_width;
  RECT client;
  HFONT font;
  SIZE size;
  HDC hdc;

  GetClientRect(infoPtr->hwndSelf, &client);

  hdc = GetDC(infoPtr->hwndSelf);
  font = SelectObject(hdc, infoPtr->hFont);

  /* get the height and width of each day's text */
  GetTextMetricsW(hdc, &tm);
  infoPtr->textHeight = tm.tmHeight + tm.tmExternalLeading + tm.tmInternalLeading;

  /* find widest day name for current locale and font */
  day_width = 0;
  for (i = 0; i < 7; i++)
  {
      SIZE sz;

      if (get_localized_dayname(infoPtr, i, buff, ARRAY_SIZE(buff)))
      {
          GetTextExtentPoint32W(hdc, buff, lstrlenW(buff), &sz);
          if (sz.cx > day_width) day_width = sz.cx;
      }
      else /* locale independent fallback on failure */
      {
          static const WCHAR sunW[] = { 'S','u','n' };
          GetTextExtentPoint32W(hdc, sunW, ARRAY_SIZE(sunW), &sz);
          day_width = sz.cx;
          break;
      }
  }

  day_width += 2;

  /* recalculate the height and width increments and offsets */
  size.cx = 0;
  GetTextExtentPoint32W(hdc, O0W, 2, &size);

  /* restore the originally selected font */
  SelectObject(hdc, font);
  ReleaseDC(infoPtr->hwndSelf, hdc);

  xdiv = (infoPtr->dwStyle & MCS_WEEKNUMBERS) ? 8 : 7;

  infoPtr->width_increment  = max(day_width, size.cx * 2 + 4);
  infoPtr->height_increment = infoPtr->textHeight;

  /* calculate title area */
  title->top    = 0;
  title->bottom = 3 * infoPtr->height_increment / 2;
  title->left   = 0;
  title->right  = infoPtr->width_increment * xdiv;

  /* set the dimensions of the next and previous buttons and center */
  /* the month text vertically */
  prev->top    = next->top    = title->top + 4;
  prev->bottom = next->bottom = title->bottom - 4;
  prev->left   = title->left + 4;
  prev->right  = prev->left + (title->bottom - title->top);
  next->right  = title->right - 4;
  next->left   = next->right - (title->bottom - title->top);

  /* titlemonth->left and right change based upon the current month
     and are recalculated in refresh as the current month may change
     without the control being resized */
  titlemonth->top    = titleyear->top    = title->top    + (infoPtr->height_increment)/2;
  titlemonth->bottom = titleyear->bottom = title->bottom - (infoPtr->height_increment)/2;

  /* week numbers */
  weeknumrect->left  = 0;
  weeknumrect->right = infoPtr->dwStyle & MCS_WEEKNUMBERS ? prev->right : 0;

  /* days abbreviated names */
  wdays->left   = days->left   = weeknumrect->right;
  wdays->right  = days->right  = wdays->left + 7 * infoPtr->width_increment;
  wdays->top    = title->bottom;
  wdays->bottom = wdays->top + infoPtr->height_increment;

  days->top    = weeknumrect->top = wdays->bottom;
  days->bottom = weeknumrect->bottom = days->top + 6 * infoPtr->height_increment;

  todayrect->left   = 0;
  todayrect->right  = title->right;
  todayrect->top    = days->bottom;
  todayrect->bottom = days->bottom + infoPtr->height_increment;

  /* compute calendar count, update all calendars */
  x = (client.right  + MC_CALENDAR_PADDING) / (title->right - title->left + MC_CALENDAR_PADDING);
  /* today label affects whole height */
  if (infoPtr->dwStyle & MCS_NOTODAY)
    y = (client.bottom + MC_CALENDAR_PADDING) / (days->bottom - title->top + MC_CALENDAR_PADDING);
  else
    y = (client.bottom - todayrect->bottom + todayrect->top + MC_CALENDAR_PADDING) /
         (days->bottom - title->top + MC_CALENDAR_PADDING);

  /* TODO: ensure that count is properly adjusted to fit 12 months constraint */
  if (x == 0) x = 1;
  if (y == 0) y = 1;

  if (x*y != MONTHCAL_GetCalCount(infoPtr))
  {
      infoPtr->dim.cx = x;
      infoPtr->dim.cy = y;
      infoPtr->calendars = heap_realloc(infoPtr->calendars, MONTHCAL_GetCalCount(infoPtr)*sizeof(CALENDAR_INFO));

      infoPtr->monthdayState = heap_realloc(infoPtr->monthdayState,
          MONTHCAL_GetMonthRange(infoPtr, GMR_DAYSTATE, 0)*sizeof(MONTHDAYSTATE));
      MONTHCAL_NotifyDayState(infoPtr);

      /* update pointers that we'll need */
      title = &infoPtr->calendars[0].title;
      wdays = &infoPtr->calendars[0].wdays;
      days  = &infoPtr->calendars[0].days;
  }

  for (i = 1; i < MONTHCAL_GetCalCount(infoPtr); i++)
  {
      /* set months */
      infoPtr->calendars[i] = infoPtr->calendars[0];
      MONTHCAL_GetMonth(&infoPtr->calendars[i].month, i);
  }

  /* offset all rectangles to center in client area */
  c_dx = (client.right  - x * title->right - MC_CALENDAR_PADDING * (x-1)) / 2;
  c_dy = (client.bottom - y * todayrect->bottom - MC_CALENDAR_PADDING * (y-1)) / 2;

  /* if calendar doesn't fit client area show it at left/top bounds */
  if (title->left + c_dx < 0) c_dx = 0;
  if (title->top  + c_dy < 0) c_dy = 0;

  for (i = 0; i < y; i++)
  {
      for (j = 0; j < x; j++)
      {
          dx = j*(title->right - title->left + MC_CALENDAR_PADDING) + c_dx;
          dy = i*(days->bottom - title->top  + MC_CALENDAR_PADDING) + c_dy;

          OffsetRect(&infoPtr->calendars[i*x+j].title, dx, dy);
          OffsetRect(&infoPtr->calendars[i*x+j].titlemonth, dx, dy);
          OffsetRect(&infoPtr->calendars[i*x+j].titleyear, dx, dy);
          OffsetRect(&infoPtr->calendars[i*x+j].wdays, dx, dy);
          OffsetRect(&infoPtr->calendars[i*x+j].weeknums, dx, dy);
          OffsetRect(&infoPtr->calendars[i*x+j].days, dx, dy);
      }
  }

  OffsetRect(prev, c_dx, c_dy);
  OffsetRect(next, (x-1)*(title->right - title->left + MC_CALENDAR_PADDING) + c_dx, c_dy);

  i = infoPtr->dim.cx * infoPtr->dim.cy - infoPtr->dim.cx;
  todayrect->left   = infoPtr->calendars[i].title.left;
  todayrect->right  = infoPtr->calendars[i].title.right;
  todayrect->top    = infoPtr->calendars[i].days.bottom;
  todayrect->bottom = infoPtr->calendars[i].days.bottom + infoPtr->height_increment;

  TRACE("dx=%d dy=%d client[%s] title[%s] wdays[%s] days[%s] today[%s]\n",
	infoPtr->width_increment,infoPtr->height_increment,
        wine_dbgstr_rect(&client),
        wine_dbgstr_rect(title),
        wine_dbgstr_rect(wdays),
        wine_dbgstr_rect(days),
        wine_dbgstr_rect(todayrect));
}

static LRESULT MONTHCAL_Size(MONTHCAL_INFO *infoPtr, int Width, int Height)
{
  TRACE("(width=%d, height=%d)\n", Width, Height);

  MONTHCAL_UpdateSize(infoPtr);
  InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

  return 0;
}

static LRESULT MONTHCAL_GetFont(const MONTHCAL_INFO *infoPtr)
{
    return (LRESULT)infoPtr->hFont;
}

static LRESULT MONTHCAL_SetFont(MONTHCAL_INFO *infoPtr, HFONT hFont, BOOL redraw)
{
    HFONT hOldFont;
    LOGFONTW lf;

    if (!hFont) return 0;

    hOldFont = infoPtr->hFont;
    infoPtr->hFont = hFont;

    GetObjectW(infoPtr->hFont, sizeof(lf), &lf);
    lf.lfWeight = FW_BOLD;
    infoPtr->hBoldFont = CreateFontIndirectW(&lf);

    MONTHCAL_UpdateSize(infoPtr);

    if (redraw)
        InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

    return (LRESULT)hOldFont;
}

/* update theme after a WM_THEMECHANGED message */
static LRESULT theme_changed (const MONTHCAL_INFO* infoPtr)
{
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
    CloseThemeData (theme);
    OpenThemeData (infoPtr->hwndSelf, themeClass);
    return 0;
}

static INT MONTHCAL_StyleChanged(MONTHCAL_INFO *infoPtr, WPARAM wStyleType,
                                 const STYLESTRUCT *lpss)
{
    TRACE("(styletype=%lx, styleOld=0x%08x, styleNew=0x%08x)\n",
          wStyleType, lpss->styleOld, lpss->styleNew);

    if (wStyleType != GWL_STYLE) return 0;

    infoPtr->dwStyle = lpss->styleNew;

    /* make room for week numbers */
    if ((lpss->styleNew ^ lpss->styleOld) & (MCS_WEEKNUMBERS | MCS_SHORTDAYSOFWEEK))
        MONTHCAL_UpdateSize(infoPtr);

    return 0;
}

static INT MONTHCAL_StyleChanging(MONTHCAL_INFO *infoPtr, WPARAM wStyleType,
                                  STYLESTRUCT *lpss)
{
    TRACE("(styletype=%lx, styleOld=0x%08x, styleNew=0x%08x)\n",
          wStyleType, lpss->styleOld, lpss->styleNew);

    /* block MCS_MULTISELECT change */
    if ((lpss->styleNew ^ lpss->styleOld) & MCS_MULTISELECT)
    {
        if (lpss->styleOld & MCS_MULTISELECT)
            lpss->styleNew |= MCS_MULTISELECT;
        else
            lpss->styleNew &= ~MCS_MULTISELECT;
    }

    /* block MCS_DAYSTATE change */
    if ((lpss->styleNew ^ lpss->styleOld) & MCS_DAYSTATE)
    {
        if (lpss->styleOld & MCS_DAYSTATE)
            lpss->styleNew |= MCS_DAYSTATE;
        else
            lpss->styleNew &= ~MCS_DAYSTATE;
    }

    return 0;
}

/* FIXME: check whether dateMin/dateMax need to be adjusted. */
static LRESULT
MONTHCAL_Create(HWND hwnd, LPCREATESTRUCTW lpcs)
{
  MONTHCAL_INFO *infoPtr;

  /* allocate memory for info structure */
  infoPtr = heap_alloc_zero(sizeof(*infoPtr));
  SetWindowLongPtrW(hwnd, 0, (DWORD_PTR)infoPtr);

  if (infoPtr == NULL) {
    ERR("could not allocate info memory!\n");
    return 0;
  }

  infoPtr->hwndSelf = hwnd;
  infoPtr->hwndNotify = lpcs->hwndParent;
  infoPtr->dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
  infoPtr->dim.cx = infoPtr->dim.cy = 1;
  infoPtr->calendars = heap_alloc_zero(sizeof(CALENDAR_INFO));
  if (!infoPtr->calendars) goto fail;
  infoPtr->monthdayState = heap_alloc_zero(3 * sizeof(MONTHDAYSTATE));
  if (!infoPtr->monthdayState) goto fail;

  /* initialize info structure */
  /* FIXME: calculate systemtime ->> localtime(subtract timezoneinfo) */

  GetLocalTime(&infoPtr->todaysDate);
  MONTHCAL_SetFirstDayOfWeek(infoPtr, -1);

  infoPtr->maxSelCount   = (infoPtr->dwStyle & MCS_MULTISELECT) ? 7 : 1;

  infoPtr->colors[MCSC_BACKGROUND]   = comctl32_color.clrWindow;
  infoPtr->colors[MCSC_TEXT]         = comctl32_color.clrWindowText;
  infoPtr->colors[MCSC_TITLEBK]      = comctl32_color.clrActiveCaption;
  infoPtr->colors[MCSC_TITLETEXT]    = comctl32_color.clrWindow;
  infoPtr->colors[MCSC_MONTHBK]      = comctl32_color.clrWindow;
  infoPtr->colors[MCSC_TRAILINGTEXT] = comctl32_color.clrGrayText;

  infoPtr->brushes[BrushBackground]  = CreateSolidBrush(infoPtr->colors[MCSC_BACKGROUND]);
  infoPtr->brushes[BrushTitle]       = CreateSolidBrush(infoPtr->colors[MCSC_TITLEBK]);
  infoPtr->brushes[BrushMonth]       = CreateSolidBrush(infoPtr->colors[MCSC_MONTHBK]);

  infoPtr->pens[PenRed]  = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
  infoPtr->pens[PenText] = CreatePen(PS_SOLID, 1, infoPtr->colors[MCSC_TEXT]);

  infoPtr->minSel = infoPtr->todaysDate;
  infoPtr->maxSel = infoPtr->todaysDate;
  infoPtr->calendars[0].month = infoPtr->todaysDate;
  infoPtr->isUnicode = TRUE;

  /* setup control layout and day state data */
  MONTHCAL_UpdateSize(infoPtr);

  /* today auto update timer, to be freed only on control destruction */
  SetTimer(infoPtr->hwndSelf, MC_TODAYUPDATETIMER, MC_TODAYUPDATEDELAY, 0);

  OpenThemeData (infoPtr->hwndSelf, themeClass);

  return 0;

fail:
  heap_free(infoPtr->monthdayState);
  heap_free(infoPtr->calendars);
  heap_free(infoPtr);
  return 0;
}

static LRESULT
MONTHCAL_Destroy(MONTHCAL_INFO *infoPtr)
{
  INT i;

  /* free month calendar info data */
  heap_free(infoPtr->monthdayState);
  heap_free(infoPtr->calendars);
  SetWindowLongPtrW(infoPtr->hwndSelf, 0, 0);

  CloseThemeData (GetWindowTheme (infoPtr->hwndSelf));

  for (i = 0; i < BrushLast; i++) DeleteObject(infoPtr->brushes[i]);
  for (i = 0; i < PenLast; i++) DeleteObject(infoPtr->pens[i]);

  heap_free(infoPtr);
  return 0;
}

/*
 * Handler for WM_NOTIFY messages
 */
static LRESULT
MONTHCAL_Notify(MONTHCAL_INFO *infoPtr, NMHDR *hdr)
{
  /* notification from year edit updown */
  if (hdr->code == UDN_DELTAPOS)
  {
    NMUPDOWN *nmud = (NMUPDOWN*)hdr;

    if (hdr->hwndFrom == infoPtr->hWndYearUpDown && nmud->iDelta)
    {
      /* year value limits are set up explicitly after updown creation */
      MONTHCAL_Scroll(infoPtr, 12 * nmud->iDelta, FALSE);
      MONTHCAL_NotifyDayState(infoPtr);
      MONTHCAL_NotifySelectionChange(infoPtr);
    }
  }
  return 0;
}

static inline BOOL
MONTHCAL_SetUnicodeFormat(MONTHCAL_INFO *infoPtr, BOOL isUnicode)
{
  BOOL prev = infoPtr->isUnicode;
  infoPtr->isUnicode = isUnicode;
  return prev;
}

static inline BOOL
MONTHCAL_GetUnicodeFormat(const MONTHCAL_INFO *infoPtr)
{
  return infoPtr->isUnicode;
}

static LRESULT WINAPI
MONTHCAL_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = (MONTHCAL_INFO *)GetWindowLongPtrW(hwnd, 0);

  TRACE("hwnd=%p msg=%x wparam=%lx lparam=%lx\n", hwnd, uMsg, wParam, lParam);

  if (!infoPtr && (uMsg != WM_CREATE))
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
  switch(uMsg)
  {
  case MCM_GETCURSEL:
    return MONTHCAL_GetCurSel(infoPtr, (LPSYSTEMTIME)lParam);

  case MCM_SETCURSEL:
    return MONTHCAL_SetCurSel(infoPtr, (LPSYSTEMTIME)lParam);

  case MCM_GETMAXSELCOUNT:
    return MONTHCAL_GetMaxSelCount(infoPtr);

  case MCM_SETMAXSELCOUNT:
    return MONTHCAL_SetMaxSelCount(infoPtr, wParam);

  case MCM_GETSELRANGE:
    return MONTHCAL_GetSelRange(infoPtr, (LPSYSTEMTIME)lParam);

  case MCM_SETSELRANGE:
    return MONTHCAL_SetSelRange(infoPtr, (LPSYSTEMTIME)lParam);

  case MCM_GETMONTHRANGE:
    return MONTHCAL_GetMonthRange(infoPtr, wParam, (SYSTEMTIME*)lParam);

  case MCM_SETDAYSTATE:
    return MONTHCAL_SetDayState(infoPtr, (INT)wParam, (LPMONTHDAYSTATE)lParam);

  case MCM_GETMINREQRECT:
    return MONTHCAL_GetMinReqRect(infoPtr, (LPRECT)lParam);

  case MCM_GETCOLOR:
    return MONTHCAL_GetColor(infoPtr, wParam);

  case MCM_SETCOLOR:
    return MONTHCAL_SetColor(infoPtr, wParam, (COLORREF)lParam);

  case MCM_GETTODAY:
    return MONTHCAL_GetToday(infoPtr, (LPSYSTEMTIME)lParam);

  case MCM_SETTODAY:
    return MONTHCAL_SetToday(infoPtr, (LPSYSTEMTIME)lParam);

  case MCM_HITTEST:
    return MONTHCAL_HitTest(infoPtr, (PMCHITTESTINFO)lParam);

  case MCM_GETFIRSTDAYOFWEEK:
    return MONTHCAL_GetFirstDayOfWeek(infoPtr);

  case MCM_SETFIRSTDAYOFWEEK:
    return MONTHCAL_SetFirstDayOfWeek(infoPtr, (INT)lParam);

  case MCM_GETRANGE:
    return MONTHCAL_GetRange(infoPtr, (LPSYSTEMTIME)lParam);

  case MCM_SETRANGE:
    return MONTHCAL_SetRange(infoPtr, (SHORT)wParam, (LPSYSTEMTIME)lParam);

  case MCM_GETMONTHDELTA:
    return MONTHCAL_GetMonthDelta(infoPtr);

  case MCM_SETMONTHDELTA:
    return MONTHCAL_SetMonthDelta(infoPtr, wParam);

  case MCM_GETMAXTODAYWIDTH:
    return MONTHCAL_GetMaxTodayWidth(infoPtr);

  case MCM_SETUNICODEFORMAT:
    return MONTHCAL_SetUnicodeFormat(infoPtr, (BOOL)wParam);

  case MCM_GETUNICODEFORMAT:
    return MONTHCAL_GetUnicodeFormat(infoPtr);

  case MCM_GETCALENDARCOUNT:
    return MONTHCAL_GetCalCount(infoPtr);

  case WM_GETDLGCODE:
    return DLGC_WANTARROWS | DLGC_WANTCHARS;

  case WM_RBUTTONUP:
    return MONTHCAL_RButtonUp(infoPtr, lParam);

  case WM_LBUTTONDOWN:
    return MONTHCAL_LButtonDown(infoPtr, lParam);

  case WM_MOUSEMOVE:
    return MONTHCAL_MouseMove(infoPtr, lParam);

  case WM_LBUTTONUP:
    return MONTHCAL_LButtonUp(infoPtr, lParam);

  case WM_PAINT:
    return MONTHCAL_Paint(infoPtr, (HDC)wParam);

  case WM_PRINTCLIENT:
    return MONTHCAL_PrintClient(infoPtr, (HDC)wParam, (DWORD)lParam);

  case WM_ERASEBKGND:
    return MONTHCAL_EraseBkgnd(infoPtr, (HDC)wParam);

  case WM_SETFOCUS:
    return MONTHCAL_SetFocus(infoPtr);

  case WM_SIZE:
    return MONTHCAL_Size(infoPtr, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

  case WM_NOTIFY:
    return MONTHCAL_Notify(infoPtr, (NMHDR*)lParam);

  case WM_CREATE:
    return MONTHCAL_Create(hwnd, (LPCREATESTRUCTW)lParam);

  case WM_SETFONT:
    return MONTHCAL_SetFont(infoPtr, (HFONT)wParam, (BOOL)lParam);

  case WM_GETFONT:
    return MONTHCAL_GetFont(infoPtr);

  case WM_TIMER:
    return MONTHCAL_Timer(infoPtr, wParam);
    
  case WM_THEMECHANGED:
    return theme_changed (infoPtr);

  case WM_DESTROY:
    return MONTHCAL_Destroy(infoPtr);

  case WM_SYSCOLORCHANGE:
    COMCTL32_RefreshSysColors();
    return 0;

  case WM_STYLECHANGED:
    return MONTHCAL_StyleChanged(infoPtr, wParam, (LPSTYLESTRUCT)lParam);

  case WM_STYLECHANGING:
    return MONTHCAL_StyleChanging(infoPtr, wParam, (LPSTYLESTRUCT)lParam);

  default:
    if ((uMsg >= WM_USER) && (uMsg < WM_APP) && !COMCTL32_IsReflectedMessage(uMsg))
      ERR( "unknown msg %04x wp=%08lx lp=%08lx\n", uMsg, wParam, lParam);
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
  }
}


void
MONTHCAL_Register(void)
{
  WNDCLASSW wndClass;

  ZeroMemory(&wndClass, sizeof(WNDCLASSW));
  wndClass.style         = CS_GLOBALCLASS;
  wndClass.lpfnWndProc   = MONTHCAL_WindowProc;
  wndClass.cbClsExtra    = 0;
  wndClass.cbWndExtra    = sizeof(MONTHCAL_INFO *);
  wndClass.hCursor       = LoadCursorW(0, (LPWSTR)IDC_ARROW);
  wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wndClass.lpszClassName = MONTHCAL_CLASSW;

  RegisterClassW(&wndClass);
}


void
MONTHCAL_Unregister(void)
{
    UnregisterClassW(MONTHCAL_CLASSW, NULL);
}
