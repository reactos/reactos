/* Month calendar control

 *
 * Copyright 1998, 1999 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * Copyright 1999 Alex Priem (alexp@sci.kun.nl)
 * Copyright 1999 Chris Morgan <cmorgan@wpi.edu> and
 *		  James Abbatiello <abbeyj@wpi.edu>
 * Copyright 2000 Uwe Bonnes <bon@elektron.ikp.physik.tu-darmstadt.de>
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
 * NOTE
 * 
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on Oct. 20, 2004, by Dimitrie O. Paun.
 * 
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features, or bugs, please note them below.
 * 
 * TODO:
 *    -- MCM_[GS]ETUNICODEFORMAT
 *    -- MONTHCAL_GetMonthRange
 *    -- handle resources better (doesn't work now); 
 *    -- take care of internationalization.
 *    -- keyboard handling.
 *    -- GetRange: At the moment, we copy ranges anyway, regardless of
 *                 infoPtr->rangeValid; an invalid range is simply filled 
 *                 with zeros in SetRange.  Is this the right behavior?
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
#include "tmschema.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(monthcal);

#define MC_SEL_LBUTUP	    1	/* Left button released */
#define MC_SEL_LBUTDOWN	    2	/* Left button pressed in calendar */
#define MC_PREVPRESSED      4   /* Prev month button pressed */
#define MC_NEXTPRESSED      8   /* Next month button pressed */
#define MC_NEXTMONTHDELAY   350	/* when continuously pressing `next */
										/* month', wait 500 ms before going */
										/* to the next month */
#define MC_NEXTMONTHTIMER   1			/* Timer ID's */
#define MC_PREVMONTHTIMER   2

#define countof(arr) (sizeof(arr)/sizeof(arr[0]))

typedef struct
{
    HWND hwndSelf;
    COLORREF	bk;
    COLORREF	txt;
    COLORREF	titlebk;
    COLORREF	titletxt;
    COLORREF	monthbk;
    COLORREF	trailingtxt;
    HFONT	hFont;
    HFONT	hBoldFont;
    int		textHeight;
    int		textWidth;
    int		height_increment;
    int		width_increment;
    int		firstDayplace; /* place of the first day of the current month */
    int		delta;	/* scroll rate; # of months that the */
                        /* control moves when user clicks a scroll button */
    int		visible;	/* # of months visible */
    int		firstDay;	/* Start month calendar with firstDay's day */
    int		firstDayHighWord;    /* High word only used externally */
    int		monthRange;
    MONTHDAYSTATE *monthdayState;
    SYSTEMTIME	todaysDate;
    DWORD	currentMonth;
    DWORD	currentYear;
    int		status;		/* See MC_SEL flags */
    int		curSelDay;	/* current selected day */
    int		firstSelDay;	/* first selected day */
    int		maxSelCount;
    SYSTEMTIME	minSel;
    SYSTEMTIME	maxSel;
    DWORD	rangeValid;
    SYSTEMTIME	minDate;
    SYSTEMTIME	maxDate;

    RECT title;		/* rect for the header above the calendar */
    RECT titlebtnnext;	/* the `next month' button in the header */
    RECT titlebtnprev;  /* the `prev month' button in the header */
    RECT titlemonth;	/* the `month name' txt in the header */
    RECT titleyear;	/* the `year number' txt in the header */
    RECT wdays;		/* week days at top */
    RECT days;		/* calendar area */
    RECT weeknums;	/* week numbers at left side */
    RECT todayrect;	/* `today: xx/xx/xx' text rect */
    HWND hwndNotify;    /* Window to receive the notifications */
    HWND hWndYearEdit;  /* Window Handle of edit box to handle years */
    HWND hWndYearUpDown;/* Window Handle of updown box to handle years */
} MONTHCAL_INFO, *LPMONTHCAL_INFO;


/* Offsets of days in the week to the weekday of january 1 in a leap year */
static const int DayOfWeekTable[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

static const WCHAR themeClass[] = { 'S','c','r','o','l','l','b','a','r',0 };

#define MONTHCAL_GetInfoPtr(hwnd) ((MONTHCAL_INFO *)GetWindowLongPtrW(hwnd, 0))

/* helper functions  */

/* returns the number of days in any given month, checking for leap days */
/* january is 1, december is 12 */
int MONTHCAL_MonthLength(int month, int year)
{
  const int mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0};
  /*Wrap around, this eases handling*/
  if(month == 0)
    month = 12;
  if(month == 13)
    month = 1;

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


/* make sure that time is valid */
static int MONTHCAL_ValidateTime(SYSTEMTIME time)
{
  if(time.wMonth < 1 || time.wMonth > 12 ) return FALSE;
  if(time.wDayOfWeek > 6) return FALSE;
  if(time.wDay > MONTHCAL_MonthLength(time.wMonth, time.wYear))
	  return FALSE;

  return TRUE;
}


/* Note:Depending on DST, this may be offset by a day.
   Need to find out if we're on a DST place & adjust the clock accordingly.
   Above function assumes we have a valid data.
   Valid for year>1752;  1 <= d <= 31, 1 <= m <= 12.
   0 = Sunday.
*/

/* returns the day in the week(0 == sunday, 6 == saturday) */
/* day(1 == 1st, 2 == 2nd... etc), year is the  year value */
static int MONTHCAL_CalculateDayOfWeek(DWORD day, DWORD month, DWORD year)
{
  year-=(month < 3);

  return((year + year/4 - year/100 + year/400 +
         DayOfWeekTable[month-1] + day ) % 7);
}

/* From a given point, calculate the row (weekpos), column(daypos)
   and day in the calendar. day== 0 mean the last day of tha last month
*/
static int MONTHCAL_CalcDayFromPos(const MONTHCAL_INFO *infoPtr, int x, int y,
				   int *daypos,int *weekpos)
{
  int retval, firstDay;
  RECT rcClient;

  GetClientRect(infoPtr->hwndSelf, &rcClient);

  /* if the point is outside the x bounds of the window put
  it at the boundary */
  if (x > rcClient.right)
    x = rcClient.right;


  *daypos = (x - infoPtr->days.left ) / infoPtr->width_increment;
  *weekpos = (y - infoPtr->days.top ) / infoPtr->height_increment;

  firstDay = (MONTHCAL_CalculateDayOfWeek(1, infoPtr->currentMonth, infoPtr->currentYear)+6 - infoPtr->firstDay)%7;
  retval = *daypos + (7 * *weekpos) - firstDay;
  return retval;
}

/* day is the day of the month, 1 == 1st day of the month */
/* sets x and y to be the position of the day */
/* x == day, y == week where(0,0) == firstDay, 1st week */
static void MONTHCAL_CalcDayXY(const MONTHCAL_INFO *infoPtr, int day, int month,
                                 int *x, int *y)
{
  int firstDay, prevMonth;

  firstDay = (MONTHCAL_CalculateDayOfWeek(1, infoPtr->currentMonth, infoPtr->currentYear) +6 - infoPtr->firstDay)%7;

  if(month==infoPtr->currentMonth) {
    *x = (day + firstDay) % 7;
    *y = (day + firstDay - *x) / 7;
    return;
  }
  if(month < infoPtr->currentMonth) {
    prevMonth = month - 1;
    if(prevMonth==0)
       prevMonth = 12;

    *x = (MONTHCAL_MonthLength(prevMonth, infoPtr->currentYear) - firstDay) % 7;
    *y = 0;
    return;
  }

  *y = MONTHCAL_MonthLength(month, infoPtr->currentYear - 1) / 7;
  *x = (day + firstDay + MONTHCAL_MonthLength(month,
       infoPtr->currentYear)) % 7;
}


/* x: column(day), y: row(week) */
static void MONTHCAL_CalcDayRect(const MONTHCAL_INFO *infoPtr, RECT *r, int x, int y)
{
  r->left = infoPtr->days.left + x * infoPtr->width_increment;
  r->right = r->left + infoPtr->width_increment;
  r->top  = infoPtr->days.top  + y * infoPtr->height_increment;
  r->bottom = r->top + infoPtr->textHeight;
}


/* sets the RECT struct r to the rectangle around the day and month */
/* day is the day value of the month(1 == 1st), month is the month */
/* value(january == 1, december == 12) */
static inline void MONTHCAL_CalcPosFromDay(const MONTHCAL_INFO *infoPtr,
                                            int day, int month, RECT *r)
{
  int x, y;

  MONTHCAL_CalcDayXY(infoPtr, day, month, &x, &y);
  MONTHCAL_CalcDayRect(infoPtr, r, x, y);
}


/* day is the day in the month(1 == 1st of the month) */
/* month is the month value(1 == january, 12 == december) */
static void MONTHCAL_CircleDay(const MONTHCAL_INFO *infoPtr, HDC hdc, int day, int month)
{
  HPEN hRedPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
  HPEN hOldPen2 = SelectObject(hdc, hRedPen);
  POINT points[13];
  int x, y;
  RECT day_rect;


  MONTHCAL_CalcPosFromDay(infoPtr, day, month, &day_rect);

  x = day_rect.left;
  y = day_rect.top;

  points[0].x = x;
  points[0].y = y - 1;
  points[1].x = x + 0.8 * infoPtr->width_increment;
  points[1].y = y - 1;
  points[2].x = x + 0.9 * infoPtr->width_increment;
  points[2].y = y;
  points[3].x = x + infoPtr->width_increment;
  points[3].y = y + 0.5 * infoPtr->height_increment;

  points[4].x = x + infoPtr->width_increment;
  points[4].y = y + 0.9 * infoPtr->height_increment;
  points[5].x = x + 0.6 * infoPtr->width_increment;
  points[5].y = y + 0.9 * infoPtr->height_increment;
  points[6].x = x + 0.5 * infoPtr->width_increment;
  points[6].y = y + 0.9 * infoPtr->height_increment; /* bring the bottom up just
				a hair to fit inside the day rectangle */

  points[7].x = x + 0.2 * infoPtr->width_increment;
  points[7].y = y + 0.8 * infoPtr->height_increment;
  points[8].x = x + 0.1 * infoPtr->width_increment;
  points[8].y = y + 0.8 * infoPtr->height_increment;
  points[9].x = x;
  points[9].y = y + 0.5 * infoPtr->height_increment;

  points[10].x = x + 0.1 * infoPtr->width_increment;
  points[10].y = y + 0.2 * infoPtr->height_increment;
  points[11].x = x + 0.2 * infoPtr->width_increment;
  points[11].y = y + 0.3 * infoPtr->height_increment;
  points[12].x = x + 0.4 * infoPtr->width_increment;
  points[12].y = y + 0.2 * infoPtr->height_increment;

  PolyBezier(hdc, points, 13);
  DeleteObject(hRedPen);
  SelectObject(hdc, hOldPen2);
}


static void MONTHCAL_DrawDay(const MONTHCAL_INFO *infoPtr, HDC hdc, int day, int month,
                             int x, int y, int bold)
{
  static const WCHAR fmtW[] = { '%','d',0 };
  WCHAR buf[10];
  RECT r;
  static int haveBoldFont, haveSelectedDay = FALSE;
  HBRUSH hbr;
  COLORREF oldCol = 0;
  COLORREF oldBk = 0;

  wsprintfW(buf, fmtW, day);

/* No need to check styles: when selection is not valid, it is set to zero.
 * 1<day<31, so evertyhing's OK.
 */

  MONTHCAL_CalcDayRect(infoPtr, &r, x, y);

  if((day>=infoPtr->minSel.wDay) && (day<=infoPtr->maxSel.wDay)
       && (month==infoPtr->currentMonth)) {
    HRGN hrgn;
    RECT r2;

    TRACE("%d %d %d\n",day, infoPtr->minSel.wDay, infoPtr->maxSel.wDay);
    TRACE("%d %d %d %d\n", r.left, r.top, r.right, r.bottom);
    oldCol = SetTextColor(hdc, infoPtr->monthbk);
    oldBk = SetBkColor(hdc, infoPtr->trailingtxt);
    hbr = GetSysColorBrush(COLOR_GRAYTEXT);
    hrgn = CreateEllipticRgn(r.left, r.top, r.right, r.bottom);
    FillRgn(hdc, hrgn, hbr);

    /* FIXME: this may need to be changed now b/c of the other
	drawing changes 11/3/99 CMM */
    r2.left   = r.left - 0.25 * infoPtr->textWidth;
    r2.top    = r.top;
    r2.right  = r.left + 0.5 * infoPtr->textWidth;
    r2.bottom = r.bottom;
    if(haveSelectedDay) FillRect(hdc, &r2, hbr);
      haveSelectedDay = TRUE;
  } else {
    haveSelectedDay = FALSE;
  }

  /* need to add some code for multiple selections */

  if((bold) &&(!haveBoldFont)) {
    SelectObject(hdc, infoPtr->hBoldFont);
    haveBoldFont = TRUE;
  }
  if((!bold) &&(haveBoldFont)) {
    SelectObject(hdc, infoPtr->hFont);
    haveBoldFont = FALSE;
  }

  if(haveSelectedDay) {
    SetTextColor(hdc, oldCol);
    SetBkColor(hdc, oldBk);
  }

  SetBkMode(hdc,TRANSPARENT);
  DrawTextW(hdc, buf, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE );

  /* draw a rectangle around the currently selected days text */
  if((day==infoPtr->curSelDay) && (month==infoPtr->currentMonth))
    DrawFocusRect(hdc, &r);
}


static void paint_button (const MONTHCAL_INFO *infoPtr, HDC hdc, BOOL btnNext,
                          BOOL pressed, RECT* r)
{
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
    
    if (theme)
    {
        static const int states[] = {
            /* Prev button */
            ABS_LEFTNORMAL,  ABS_LEFTPRESSED,  ABS_LEFTDISABLED,
            /* Next button */
            ABS_RIGHTNORMAL, ABS_RIGHTPRESSED, ABS_RIGHTDISABLED
        };
        int stateNum = btnNext ? 3 : 0;
        if (pressed)
            stateNum += 1;
        else
        {
            DWORD dwStyle = GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE);
            if (dwStyle & WS_DISABLED) stateNum += 2;
        }
        DrawThemeBackground (theme, hdc, SBP_ARROWBTN, states[stateNum], r, NULL);
    }
    else
    {
        int style = btnNext ? DFCS_SCROLLRIGHT : DFCS_SCROLLLEFT;
        if (pressed)
            style |= DFCS_PUSHED;
        else
        {
            DWORD dwStyle = GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE);
            if (dwStyle & WS_DISABLED) style |= DFCS_INACTIVE;
        }
        
        DrawFrameControl(hdc, r, DFC_SCROLL, style);
    }
}


static void MONTHCAL_Refresh(MONTHCAL_INFO *infoPtr, HDC hdc, const PAINTSTRUCT *ps)
{
  static const WCHAR todayW[] = { 'T','o','d','a','y',':',0 };
  static const WCHAR fmt1W[] = { '%','s',' ','%','l','d',0 };
  static const WCHAR fmt2W[] = { '%','s',' ','%','s',0 };
  static const WCHAR fmt3W[] = { '%','d',0 };
  RECT *title=&infoPtr->title;
  RECT *prev=&infoPtr->titlebtnprev;
  RECT *next=&infoPtr->titlebtnnext;
  RECT *titlemonth=&infoPtr->titlemonth;
  RECT *titleyear=&infoPtr->titleyear;
  RECT dayrect;
  RECT *days=&dayrect;
  RECT rtoday;
  int i, j, m, mask, day, firstDay, weeknum, weeknum1,prevMonth;
  int textHeight = infoPtr->textHeight, textWidth = infoPtr->textWidth;
  SIZE size;
  HBRUSH hbr;
  HFONT currentFont;
  WCHAR buf[20];
  WCHAR buf1[20];
  WCHAR buf2[32];
  COLORREF oldTextColor, oldBkColor;
  DWORD dwStyle = GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE);
  RECT rcTemp;
  RECT rcDay; /* used in MONTHCAL_CalcDayRect() */
  SYSTEMTIME localtime;
  int startofprescal;

  oldTextColor = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

  /* fill background */
  hbr = CreateSolidBrush (infoPtr->bk);
  FillRect(hdc, &ps->rcPaint, hbr);
  DeleteObject(hbr);

  /* draw header */
  if(IntersectRect(&rcTemp, &(ps->rcPaint), title))
  {
    hbr =  CreateSolidBrush(infoPtr->titlebk);
    FillRect(hdc, title, hbr);
    DeleteObject(hbr);
  }

  /* if the previous button is pressed draw it depressed */
  if(IntersectRect(&rcTemp, &(ps->rcPaint), prev))
    paint_button (infoPtr, hdc, FALSE, infoPtr->status & MC_PREVPRESSED, prev);

  /* if next button is depressed draw it depressed */
  if(IntersectRect(&rcTemp, &(ps->rcPaint), next))
    paint_button (infoPtr, hdc, TRUE, infoPtr->status & MC_NEXTPRESSED, next);

  oldBkColor = SetBkColor(hdc, infoPtr->titlebk);
  SetTextColor(hdc, infoPtr->titletxt);
  currentFont = SelectObject(hdc, infoPtr->hBoldFont);

  GetLocaleInfoW( LOCALE_USER_DEFAULT,LOCALE_SMONTHNAME1+infoPtr->currentMonth -1,
		  buf1,countof(buf1));
  wsprintfW(buf, fmt1W, buf1, infoPtr->currentYear);

  if(IntersectRect(&rcTemp, &(ps->rcPaint), title))
  {
    DrawTextW(hdc, buf, strlenW(buf), title,
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  }

/* titlemonth left/right contained rect for whole titletxt('June  1999')
  * MCM_HitTestInfo wants month & year rects, so prepare these now.
  *(no, we can't draw them separately; the whole text is centered)
  */
  GetTextExtentPoint32W(hdc, buf, strlenW(buf), &size);
  titlemonth->left = title->right / 2 + title->left / 2 - size.cx / 2;
  titleyear->right = title->right / 2 + title->left / 2 + size.cx / 2;
  GetTextExtentPoint32W(hdc, buf1, strlenW(buf1), &size);
  titlemonth->right = titlemonth->left + size.cx;
  titleyear->left = titlemonth->right;

  /* draw month area */
  rcTemp.top=infoPtr->wdays.top;
  rcTemp.left=infoPtr->wdays.left;
  rcTemp.bottom=infoPtr->todayrect.bottom;
  rcTemp.right =infoPtr->todayrect.right;
  if(IntersectRect(&rcTemp, &(ps->rcPaint), &rcTemp))
  {
    hbr =  CreateSolidBrush(infoPtr->monthbk);
    FillRect(hdc, &rcTemp, hbr);
    DeleteObject(hbr);
  }

/* draw line under day abbreviatons */

  MoveToEx(hdc, infoPtr->days.left + 3, title->bottom + textHeight + 1, NULL);
  LineTo(hdc, infoPtr->days.right - 3, title->bottom + textHeight + 1);

  prevMonth = infoPtr->currentMonth - 1;
  if(prevMonth == 0) /* if currentMonth is january(1) prevMonth is */
    prevMonth = 12;    /* december(12) of the previous year */

  infoPtr->wdays.left   = infoPtr->days.left   = infoPtr->weeknums.right;
/* draw day abbreviations */

  SelectObject(hdc, infoPtr->hFont);
  SetBkColor(hdc, infoPtr->monthbk);
  SetTextColor(hdc, infoPtr->trailingtxt);

  /* copy this rect so we can change the values without changing */
  /* the original version */
  days->left = infoPtr->wdays.left;
  days->right = days->left + infoPtr->width_increment;
  days->top = infoPtr->wdays.top;
  days->bottom = infoPtr->wdays.bottom;

  i = infoPtr->firstDay;

  for(j=0; j<7; j++) {
    GetLocaleInfoW( LOCALE_USER_DEFAULT,LOCALE_SABBREVDAYNAME1 + (i+j+6)%7, buf, countof(buf));
    DrawTextW(hdc, buf, strlenW(buf), days, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
    days->left+=infoPtr->width_increment;
    days->right+=infoPtr->width_increment;
  }

/* draw day numbers; first, the previous month */

  firstDay = MONTHCAL_CalculateDayOfWeek(1, infoPtr->currentMonth, infoPtr->currentYear);

  day = MONTHCAL_MonthLength(prevMonth, infoPtr->currentYear)  +
    (infoPtr->firstDay + 7  - firstDay)%7 + 1;
  if (day > MONTHCAL_MonthLength(prevMonth, infoPtr->currentYear))
    day -=7;
  startofprescal = day;
  mask = 1<<(day-1);

  i = 0;
  m = 0;
  while(day <= MONTHCAL_MonthLength(prevMonth, infoPtr->currentYear)) {
    MONTHCAL_CalcDayRect(infoPtr, &rcDay, i, 0);
    if(IntersectRect(&rcTemp, &(ps->rcPaint), &rcDay))
    {
      MONTHCAL_DrawDay(infoPtr, hdc, day, prevMonth, i, 0,
          infoPtr->monthdayState[m] & mask);
    }

    mask<<=1;
    day++;
    i++;
  }

/* draw `current' month  */

  day = 1; /* start at the beginning of the current month */

  infoPtr->firstDayplace = i;
  SetTextColor(hdc, infoPtr->txt);
  m++;
  mask = 1;

  /* draw the first week of the current month */
  while(i<7) {
    MONTHCAL_CalcDayRect(infoPtr, &rcDay, i, 0);
    if(IntersectRect(&rcTemp, &(ps->rcPaint), &rcDay))
    {

      MONTHCAL_DrawDay(infoPtr, hdc, day, infoPtr->currentMonth, i, 0,
	infoPtr->monthdayState[m] & mask);

      if((infoPtr->currentMonth==infoPtr->todaysDate.wMonth) &&
          (day==infoPtr->todaysDate.wDay) &&
	  (infoPtr->currentYear == infoPtr->todaysDate.wYear)) {
        if(!(dwStyle & MCS_NOTODAYCIRCLE))
	  MONTHCAL_CircleDay(infoPtr, hdc, day, infoPtr->currentMonth);
      }
    }

    mask<<=1;
    day++;
    i++;
  }

  j = 1; /* move to the 2nd week of the current month */
  i = 0; /* move back to sunday */
  while(day <= MONTHCAL_MonthLength(infoPtr->currentMonth, infoPtr->currentYear)) {
    MONTHCAL_CalcDayRect(infoPtr, &rcDay, i, j);
    if(IntersectRect(&rcTemp, &(ps->rcPaint), &rcDay))
    {
      MONTHCAL_DrawDay(infoPtr, hdc, day, infoPtr->currentMonth, i, j,
          infoPtr->monthdayState[m] & mask);

      if((infoPtr->currentMonth==infoPtr->todaysDate.wMonth) &&
          (day==infoPtr->todaysDate.wDay) &&
          (infoPtr->currentYear == infoPtr->todaysDate.wYear))
        if(!(dwStyle & MCS_NOTODAYCIRCLE))
	  MONTHCAL_CircleDay(infoPtr, hdc, day, infoPtr->currentMonth);
    }
    mask<<=1;
    day++;
    i++;
    if(i>6) { /* past saturday, goto the next weeks sunday */
      i = 0;
      j++;
    }
  }

/*  draw `next' month */

  day = 1; /* start at the first day of the next month */
  m++;
  mask = 1;

  SetTextColor(hdc, infoPtr->trailingtxt);
  while((i<7) &&(j<6)) {
    MONTHCAL_CalcDayRect(infoPtr, &rcDay, i, j);
    if(IntersectRect(&rcTemp, &(ps->rcPaint), &rcDay))
    {
      MONTHCAL_DrawDay(infoPtr, hdc, day, infoPtr->currentMonth + 1, i, j,
		infoPtr->monthdayState[m] & mask);
    }

    mask<<=1;
    day++;
    i++;
    if(i==7) { /* past saturday, go to next week's sunday */
      i = 0;
      j++;
    }
  }
  SetTextColor(hdc, infoPtr->txt);


/* draw `today' date if style allows it, and draw a circle before today's
 * date if necessary */

  if(!(dwStyle & MCS_NOTODAY))  {
    int offset = 0;
    if(!(dwStyle & MCS_NOTODAYCIRCLE))  {
      /*day is the number of days from nextmonth we put on the calendar */
      MONTHCAL_CircleDay(infoPtr, hdc,
			 day+MONTHCAL_MonthLength(infoPtr->currentMonth,infoPtr->currentYear),
			 infoPtr->currentMonth);
      offset+=textWidth;
    }
    if (!LoadStringW(COMCTL32_hModule,IDM_TODAY,buf1,countof(buf1)))
      {
	WARN("Can't load resource\n");
	strcpyW(buf1, todayW);
      }
    MONTHCAL_CalcDayRect(infoPtr, &rtoday, 1, 6);
    MONTHCAL_CopyTime(&infoPtr->todaysDate,&localtime);
    GetDateFormatW(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&localtime,NULL,buf2,countof(buf2));
    wsprintfW(buf, fmt2W, buf1, buf2);
    SelectObject(hdc, infoPtr->hBoldFont);

    DrawTextW(hdc, buf, -1, &rtoday, DT_CALCRECT | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    if(IntersectRect(&rcTemp, &(ps->rcPaint), &rtoday))
    {
      DrawTextW(hdc, buf, -1, &rtoday, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    SelectObject(hdc, infoPtr->hFont);
  }

/*eventually draw week numbers*/
  if(dwStyle & MCS_WEEKNUMBERS)  {
    /* display weeknumbers*/
    int mindays;

    /* Rules what week to call the first week of a new year:
       LOCALE_IFIRSTWEEKOFYEAR == 0 (e.g US?):
       The week containing Jan 1 is the first week of year
       LOCALE_IFIRSTWEEKOFYEAR == 2 (e.g. Germany):
       First week of year must contain 4 days of the new year
       LOCALE_IFIRSTWEEKOFYEAR == 1  (what contries?)
       The first week of the year must contain only days of the new year
    */
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IFIRSTWEEKOFYEAR, buf, countof(buf));
    weeknum = atoiW(buf);
    switch (weeknum)
      {
      case 1: mindays = 6;
	break;
      case 2: mindays = 3;
	break;
      case 0:
      default:
	mindays = 0;
      }
    if (infoPtr->currentMonth < 2)
      {
	/* calculate all those exceptions for january */
	weeknum1=MONTHCAL_CalculateDayOfWeek(1,1,infoPtr->currentYear);
	if ((infoPtr->firstDay +7 - weeknum1)%7 > mindays)
	    weeknum =1;
	else
	  {
	    weeknum = 0;
	    for(i=0; i<11; i++)
	      weeknum+=MONTHCAL_MonthLength(i+1, infoPtr->currentYear-1);
	    weeknum +=startofprescal+ 7;
	    weeknum /=7;
	    weeknum1=MONTHCAL_CalculateDayOfWeek(1,1,infoPtr->currentYear-1);
	    if ((infoPtr->firstDay + 7 - weeknum1)%7 > mindays)
	      weeknum++;
	  }
      }
    else
      {
	weeknum = 0;
	for(i=0; i<prevMonth-1; i++)
	  weeknum+=MONTHCAL_MonthLength(i+1, infoPtr->currentYear);
	weeknum +=startofprescal+ 7;
	weeknum /=7;
	weeknum1=MONTHCAL_CalculateDayOfWeek(1,1,infoPtr->currentYear);
	if ((infoPtr->firstDay + 7 - weeknum1)%7 > mindays)
	  weeknum++;
      }
    days->left = infoPtr->weeknums.left;
    days->right = infoPtr->weeknums.right;
    days->top = infoPtr->weeknums.top;
    days->bottom = days->top +infoPtr->height_increment;
    for(i=0; i<6; i++) {
      if((i==0)&&(weeknum>50))
	{
	  wsprintfW(buf, fmt3W, weeknum);
	  weeknum=0;
	}
      else if((i==5)&&(weeknum>47))
	{
	  wsprintfW(buf, fmt3W, 1);
	}
      else
	wsprintfW(buf, fmt3W, weeknum + i);
      DrawTextW(hdc, buf, -1, days, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
      days->top+=infoPtr->height_increment;
      days->bottom+=infoPtr->height_increment;
    }

    MoveToEx(hdc, infoPtr->weeknums.right, infoPtr->weeknums.top + 3 , NULL);
    LineTo(hdc,   infoPtr->weeknums.right, infoPtr->weeknums.bottom );

  }
  /* currentFont was font at entering Refresh */

  SetBkColor(hdc, oldBkColor);
  SelectObject(hdc, currentFont);
  SetTextColor(hdc, oldTextColor);
}


static LRESULT
MONTHCAL_GetMinReqRect(const MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  LPRECT lpRect = (LPRECT) lParam;

  TRACE("rect %p\n", lpRect);

  /* validate parameters */

  if((infoPtr==NULL) ||(lpRect == NULL) ) return FALSE;

  lpRect->left = infoPtr->title.left;
  lpRect->top = infoPtr->title.top;
  lpRect->right = infoPtr->title.right;
  lpRect->bottom = infoPtr->todayrect.bottom;
  AdjustWindowRect(lpRect, GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE), FALSE);

  TRACE("%s\n", wine_dbgstr_rect(lpRect));

  return TRUE;
}


static LRESULT
MONTHCAL_GetColor(const MONTHCAL_INFO *infoPtr, WPARAM wParam)
{
  TRACE("\n");

  switch((int)wParam) {
    case MCSC_BACKGROUND:
      return infoPtr->bk;
    case MCSC_TEXT:
      return infoPtr->txt;
    case MCSC_TITLEBK:
      return infoPtr->titlebk;
    case MCSC_TITLETEXT:
      return infoPtr->titletxt;
    case MCSC_MONTHBK:
      return infoPtr->monthbk;
    case MCSC_TRAILINGTEXT:
      return infoPtr->trailingtxt;
  }

  return -1;
}


static LRESULT
MONTHCAL_SetColor(MONTHCAL_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
  int prev = -1;

  TRACE("%ld: color %08lx\n", wParam, lParam);

  switch((int)wParam) {
    case MCSC_BACKGROUND:
      prev = infoPtr->bk;
      infoPtr->bk = (COLORREF)lParam;
      break;
    case MCSC_TEXT:
      prev = infoPtr->txt;
      infoPtr->txt = (COLORREF)lParam;
      break;
    case MCSC_TITLEBK:
      prev = infoPtr->titlebk;
      infoPtr->titlebk = (COLORREF)lParam;
      break;
    case MCSC_TITLETEXT:
      prev=infoPtr->titletxt;
      infoPtr->titletxt = (COLORREF)lParam;
      break;
    case MCSC_MONTHBK:
      prev = infoPtr->monthbk;
      infoPtr->monthbk = (COLORREF)lParam;
      break;
    case MCSC_TRAILINGTEXT:
      prev = infoPtr->trailingtxt;
      infoPtr->trailingtxt = (COLORREF)lParam;
      break;
  }

  InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
  return prev;
}


static LRESULT
MONTHCAL_GetMonthDelta(const MONTHCAL_INFO *infoPtr)
{
  TRACE("\n");

  if(infoPtr->delta)
    return infoPtr->delta;
  else
    return infoPtr->visible;
}


static LRESULT
MONTHCAL_SetMonthDelta(MONTHCAL_INFO *infoPtr, WPARAM wParam)
{
  int prev = infoPtr->delta;

  TRACE("delta %ld\n", wParam);

  infoPtr->delta = (int)wParam;
  return prev;
}


static LRESULT
MONTHCAL_GetFirstDayOfWeek(const MONTHCAL_INFO *infoPtr)
{
  return MAKELONG(infoPtr->firstDay, infoPtr->firstDayHighWord);
}


/* sets the first day of the week that will appear in the control */
/* 0 == Sunday, 6 == Saturday */
/* FIXME: this needs to be implemented properly in MONTHCAL_Refresh() */
/* FIXME: we need more error checking here */
static LRESULT
MONTHCAL_SetFirstDayOfWeek(MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  int prev = MAKELONG(infoPtr->firstDay, infoPtr->firstDayHighWord);
  int localFirstDay;
  WCHAR buf[40];

  TRACE("day %ld\n", lParam);

  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, buf, countof(buf));
  TRACE("%s %d\n", debugstr_w(buf), strlenW(buf));

  localFirstDay = atoiW(buf);

  if(lParam == -1)
  {
    infoPtr->firstDay = localFirstDay;
    infoPtr->firstDayHighWord = FALSE;
  }
  else if(lParam >= 7)
  {
    infoPtr->firstDay = localFirstDay;
    infoPtr->firstDayHighWord = TRUE;
  }
  else
  {
    infoPtr->firstDay = lParam;
    infoPtr->firstDayHighWord = TRUE;
  }

  return prev;
}


static LRESULT
MONTHCAL_GetMonthRange(const MONTHCAL_INFO *infoPtr)
{
  TRACE("\n");

  return infoPtr->monthRange;
}


static LRESULT
MONTHCAL_GetMaxTodayWidth(const MONTHCAL_INFO *infoPtr)
{
  return(infoPtr->todayrect.right - infoPtr->todayrect.left);
}


static LRESULT
MONTHCAL_SetRange(MONTHCAL_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    SYSTEMTIME *lprgSysTimeArray=(SYSTEMTIME *)lParam;
    FILETIME ft_min, ft_max;

    TRACE("%lx %lx\n", wParam, lParam);

    if ((wParam & GDTR_MIN && !MONTHCAL_ValidateTime(lprgSysTimeArray[0])) ||
        (wParam & GDTR_MAX && !MONTHCAL_ValidateTime(lprgSysTimeArray[1])))
        return FALSE;

    if (wParam & GDTR_MIN)
    {
        MONTHCAL_CopyTime(&lprgSysTimeArray[0], &infoPtr->minDate);
        infoPtr->rangeValid |= GDTR_MIN;
    }
    if (wParam & GDTR_MAX)
    {
        MONTHCAL_CopyTime(&lprgSysTimeArray[1], &infoPtr->maxDate);
        infoPtr->rangeValid |= GDTR_MAX;
    }

    /* Only one limit set - we are done */
    if ((infoPtr->rangeValid & (GDTR_MIN | GDTR_MAX)) != (GDTR_MIN | GDTR_MAX))
        return TRUE;
    
    SystemTimeToFileTime(&infoPtr->maxDate, &ft_max);
    SystemTimeToFileTime(&infoPtr->minDate, &ft_min);

    if (CompareFileTime(&ft_min, &ft_max) > 0)
    {
        if ((wParam & (GDTR_MIN | GDTR_MAX)) == (GDTR_MIN | GDTR_MAX))
        {
            /* Native swaps limits only when both limits are being set. */
            SYSTEMTIME st_tmp = infoPtr->minDate;
            infoPtr->minDate  = infoPtr->maxDate;
            infoPtr->maxDate  = st_tmp;
        }
        else
        {
            /* Reset the other limit. */
            /* FIXME: native sets date&time to 0. Should we do this too? */
            infoPtr->rangeValid &= wParam & GDTR_MIN ? ~GDTR_MAX : ~GDTR_MIN ;
        }
    }

    return TRUE;
}


static LRESULT
MONTHCAL_GetRange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lprgSysTimeArray = (SYSTEMTIME *)lParam;

  /* validate parameters */

  if((infoPtr==NULL) || (lprgSysTimeArray==NULL)) return FALSE;

  MONTHCAL_CopyTime(&infoPtr->maxDate, &lprgSysTimeArray[1]);
  MONTHCAL_CopyTime(&infoPtr->minDate, &lprgSysTimeArray[0]);

  return infoPtr->rangeValid;
}


static LRESULT
MONTHCAL_SetDayState(const MONTHCAL_INFO *infoPtr, WPARAM wParam, LPARAM lParam)

{
  int i, iMonths = (int)wParam;
  MONTHDAYSTATE *dayStates = (LPMONTHDAYSTATE)lParam;

  TRACE("%lx %lx\n", wParam, lParam);
  if(iMonths!=infoPtr->monthRange) return 0;

  for(i=0; i<iMonths; i++)
    infoPtr->monthdayState[i] = dayStates[i];
  return 1;
}

static LRESULT
MONTHCAL_GetCurSel(const MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  SYSTEMTIME *lpSel = (SYSTEMTIME *) lParam;

  TRACE("%lx\n", lParam);
  if((infoPtr==NULL) ||(lpSel==NULL)) return FALSE;
  if(GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE) & MCS_MULTISELECT) return FALSE;

  MONTHCAL_CopyTime(&infoPtr->minSel, lpSel);
  TRACE("%d/%d/%d\n", lpSel->wYear, lpSel->wMonth, lpSel->wDay);
  return TRUE;
}

/* FIXME: if the specified date is not visible, make it visible */
/* FIXME: redraw? */
static LRESULT
MONTHCAL_SetCurSel(MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  SYSTEMTIME *lpSel = (SYSTEMTIME *)lParam;

  TRACE("%lx\n", lParam);
  if((infoPtr==NULL) ||(lpSel==NULL)) return FALSE;
  if(GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE) & MCS_MULTISELECT) return FALSE;

  if(!MONTHCAL_ValidateTime(*lpSel)) return FALSE;

  infoPtr->currentMonth=lpSel->wMonth;
  infoPtr->currentYear=lpSel->wYear;

  MONTHCAL_CopyTime(lpSel, &infoPtr->minSel);
  MONTHCAL_CopyTime(lpSel, &infoPtr->maxSel);

  InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

  return TRUE;
}


static LRESULT
MONTHCAL_GetMaxSelCount(const MONTHCAL_INFO *infoPtr)
{
  return infoPtr->maxSelCount;
}


static LRESULT
MONTHCAL_SetMaxSelCount(MONTHCAL_INFO *infoPtr, WPARAM wParam)
{
  TRACE("%lx\n", wParam);

  if(GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE) & MCS_MULTISELECT)  {
    infoPtr->maxSelCount = wParam;
  }

  return TRUE;
}


static LRESULT
MONTHCAL_GetSelRange(const MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  SYSTEMTIME *lprgSysTimeArray = (SYSTEMTIME *) lParam;

  TRACE("%lx\n", lParam);

  /* validate parameters */

  if((infoPtr==NULL) ||(lprgSysTimeArray==NULL)) return FALSE;

  if(GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE) & MCS_MULTISELECT)
  {
    MONTHCAL_CopyTime(&infoPtr->maxSel, &lprgSysTimeArray[1]);
    MONTHCAL_CopyTime(&infoPtr->minSel, &lprgSysTimeArray[0]);
    TRACE("[min,max]=[%d %d]\n", infoPtr->minSel.wDay, infoPtr->maxSel.wDay);
    return TRUE;
  }

  return FALSE;
}


static LRESULT
MONTHCAL_SetSelRange(MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  SYSTEMTIME *lprgSysTimeArray = (SYSTEMTIME *) lParam;

  TRACE("%lx\n", lParam);

  /* validate parameters */

  if((infoPtr==NULL) ||(lprgSysTimeArray==NULL)) return FALSE;

  if(GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE) & MCS_MULTISELECT)
  {
    MONTHCAL_CopyTime(&lprgSysTimeArray[1], &infoPtr->maxSel);
    MONTHCAL_CopyTime(&lprgSysTimeArray[0], &infoPtr->minSel);
    TRACE("[min,max]=[%d %d]\n", infoPtr->minSel.wDay, infoPtr->maxSel.wDay);
    return TRUE;
  }

  return FALSE;
}


static LRESULT
MONTHCAL_GetToday(const MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  SYSTEMTIME *lpToday = (SYSTEMTIME *) lParam;

  TRACE("%lx\n", lParam);

  /* validate parameters */

  if((infoPtr==NULL) || (lpToday==NULL)) return FALSE;
  MONTHCAL_CopyTime(&infoPtr->todaysDate, lpToday);
  return TRUE;
}


static LRESULT
MONTHCAL_SetToday(MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  SYSTEMTIME *lpToday = (SYSTEMTIME *) lParam;

  TRACE("%lx\n", lParam);

  /* validate parameters */

  if((infoPtr==NULL) ||(lpToday==NULL)) return FALSE;
  MONTHCAL_CopyTime(lpToday, &infoPtr->todaysDate);
  InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
  return TRUE;
}


static LRESULT
MONTHCAL_HitTest(const MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  PMCHITTESTINFO lpht = (PMCHITTESTINFO)lParam;
  UINT x,y;
  DWORD retval;
  int day,wday,wnum;


  x = lpht->pt.x;
  y = lpht->pt.y;
  retval = MCHT_NOWHERE;

  ZeroMemory(&lpht->st, sizeof(lpht->st));

  /* Comment in for debugging...
  TRACE("%d %d wd[%d %d %d %d] d[%d %d %d %d] t[%d %d %d %d] wn[%d %d %d %d]\n", x, y,
	infoPtr->wdays.left, infoPtr->wdays.right,
	infoPtr->wdays.top, infoPtr->wdays.bottom,
	infoPtr->days.left, infoPtr->days.right,
	infoPtr->days.top, infoPtr->days.bottom,
	infoPtr->todayrect.left, infoPtr->todayrect.right,
	infoPtr->todayrect.top, infoPtr->todayrect.bottom,
	infoPtr->weeknums.left, infoPtr->weeknums.right,
	infoPtr->weeknums.top, infoPtr->weeknums.bottom);
  */

  /* are we in the header? */

  if(PtInRect(&infoPtr->title, lpht->pt)) {
    if(PtInRect(&infoPtr->titlebtnprev, lpht->pt)) {
      retval = MCHT_TITLEBTNPREV;
      goto done;
    }
    if(PtInRect(&infoPtr->titlebtnnext, lpht->pt)) {
      retval = MCHT_TITLEBTNNEXT;
      goto done;
    }
    if(PtInRect(&infoPtr->titlemonth, lpht->pt)) {
      retval = MCHT_TITLEMONTH;
      goto done;
    }
    if(PtInRect(&infoPtr->titleyear, lpht->pt)) {
      retval = MCHT_TITLEYEAR;
      goto done;
    }

    retval = MCHT_TITLE;
    goto done;
  }

  day = MONTHCAL_CalcDayFromPos(infoPtr,x,y,&wday,&wnum);
  if(PtInRect(&infoPtr->wdays, lpht->pt)) {
    retval = MCHT_CALENDARDAY;
    lpht->st.wYear  = infoPtr->currentYear;
    lpht->st.wMonth = (day < 1)? infoPtr->currentMonth -1 : infoPtr->currentMonth;
    lpht->st.wDay   = (day < 1)?
      MONTHCAL_MonthLength(infoPtr->currentMonth-1,infoPtr->currentYear) -day : day;
    goto done;
  }
  if(PtInRect(&infoPtr->weeknums, lpht->pt)) {
    retval = MCHT_CALENDARWEEKNUM;
    lpht->st.wYear  = infoPtr->currentYear;
    lpht->st.wMonth = (day < 1) ? infoPtr->currentMonth -1 :
      (day > MONTHCAL_MonthLength(infoPtr->currentMonth,infoPtr->currentYear)) ?
      infoPtr->currentMonth +1 :infoPtr->currentMonth;
    lpht->st.wDay   = (day < 1 ) ?
      MONTHCAL_MonthLength(infoPtr->currentMonth-1,infoPtr->currentYear) -day :
      (day > MONTHCAL_MonthLength(infoPtr->currentMonth,infoPtr->currentYear)) ?
      day - MONTHCAL_MonthLength(infoPtr->currentMonth,infoPtr->currentYear) : day;
    goto done;
  }
  if(PtInRect(&infoPtr->days, lpht->pt))
    {
      lpht->st.wYear  = infoPtr->currentYear;
      if ( day < 1)
	{
	  retval = MCHT_CALENDARDATEPREV;
	  lpht->st.wMonth = infoPtr->currentMonth - 1;
	  if (lpht->st.wMonth <1)
	    {
	      lpht->st.wMonth = 12;
	      lpht->st.wYear--;
	    }
	  lpht->st.wDay   = MONTHCAL_MonthLength(lpht->st.wMonth,lpht->st.wYear) -day;
	}
      else if (day > MONTHCAL_MonthLength(infoPtr->currentMonth,infoPtr->currentYear))
	{
	  retval = MCHT_CALENDARDATENEXT;
	  lpht->st.wMonth = infoPtr->currentMonth + 1;
	  if (lpht->st.wMonth <12)
	    {
	      lpht->st.wMonth = 1;
	      lpht->st.wYear++;
	    }
	  lpht->st.wDay   = day - MONTHCAL_MonthLength(infoPtr->currentMonth,infoPtr->currentYear) ;
	}
      else {
	retval = MCHT_CALENDARDATE;
	lpht->st.wMonth = infoPtr->currentMonth;
	lpht->st.wDay   = day;
	lpht->st.wDayOfWeek   = MONTHCAL_CalculateDayOfWeek(day,lpht->st.wMonth,lpht->st.wYear);
      }
      goto done;
    }
  if(PtInRect(&infoPtr->todayrect, lpht->pt)) {
    retval = MCHT_TODAYLINK;
    goto done;
  }


  /* Hit nothing special? What's left must be background :-) */

  retval = MCHT_CALENDARBK;
 done:
  lpht->uHit = retval;
  return retval;
}


static void MONTHCAL_GoToNextMonth(MONTHCAL_INFO *infoPtr)
{
  DWORD dwStyle = GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE);

  TRACE("MONTHCAL_GoToNextMonth\n");

  infoPtr->currentMonth++;
  if(infoPtr->currentMonth > 12) {
    infoPtr->currentYear++;
    infoPtr->currentMonth = 1;
  }

  if(dwStyle & MCS_DAYSTATE) {
    NMDAYSTATE nmds;
    int i;

    nmds.nmhdr.hwndFrom = infoPtr->hwndSelf;
    nmds.nmhdr.idFrom   = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
    nmds.nmhdr.code     = MCN_GETDAYSTATE;
    nmds.cDayState	= infoPtr->monthRange;
    nmds.prgDayState	= Alloc(infoPtr->monthRange * sizeof(MONTHDAYSTATE));

    SendMessageW(infoPtr->hwndNotify, WM_NOTIFY,
    (WPARAM)nmds.nmhdr.idFrom, (LPARAM)&nmds);
    for(i=0; i<infoPtr->monthRange; i++)
      infoPtr->monthdayState[i] = nmds.prgDayState[i];
  }
}


static void MONTHCAL_GoToPrevMonth(MONTHCAL_INFO *infoPtr)
{
  DWORD dwStyle = GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE);

  TRACE("\n");

  infoPtr->currentMonth--;
  if(infoPtr->currentMonth < 1) {
    infoPtr->currentYear--;
    infoPtr->currentMonth = 12;
  }

  if(dwStyle & MCS_DAYSTATE) {
    NMDAYSTATE nmds;
    int i;

    nmds.nmhdr.hwndFrom = infoPtr->hwndSelf;
    nmds.nmhdr.idFrom   = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
    nmds.nmhdr.code     = MCN_GETDAYSTATE;
    nmds.cDayState	= infoPtr->monthRange;
    nmds.prgDayState	= Alloc
                        (infoPtr->monthRange * sizeof(MONTHDAYSTATE));

    SendMessageW(infoPtr->hwndNotify, WM_NOTIFY,
        (WPARAM)nmds.nmhdr.idFrom, (LPARAM)&nmds);
    for(i=0; i<infoPtr->monthRange; i++)
       infoPtr->monthdayState[i] = nmds.prgDayState[i];
  }
}

static LRESULT
MONTHCAL_RButtonDown(MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  static const WCHAR todayW[] = { 'G','o',' ','t','o',' ','T','o','d','a','y',':',0 };
  HMENU hMenu;
  POINT menupoint;
  WCHAR buf[32];

  hMenu = CreatePopupMenu();
  if (!LoadStringW(COMCTL32_hModule,IDM_GOTODAY,buf,countof(buf)))
    {
      WARN("Can't load resource\n");
      strcpyW(buf, todayW);
    }
  AppendMenuW(hMenu, MF_STRING|MF_ENABLED,1, buf);
  menupoint.x=(short)LOWORD(lParam);
  menupoint.y=(short)HIWORD(lParam);
  ClientToScreen(infoPtr->hwndSelf, &menupoint);
  if( TrackPopupMenu(hMenu,TPM_RIGHTBUTTON| TPM_NONOTIFY|TPM_RETURNCMD,
		     menupoint.x, menupoint.y, 0, infoPtr->hwndSelf, NULL))
    {
      infoPtr->currentMonth=infoPtr->todaysDate.wMonth;
      infoPtr->currentYear=infoPtr->todaysDate.wYear;
      InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
    }
  return 0;
}

static LRESULT
MONTHCAL_LButtonDown(MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  static const WCHAR EditW[] = { 'E','D','I','T',0 };
  MCHITTESTINFO ht;
  DWORD hit;
  HMENU hMenu;
  RECT rcDay; /* used in determining area to invalidate */
  WCHAR buf[32];
  int i;
  POINT menupoint;

  TRACE("%lx\n", lParam);

  if (infoPtr->hWndYearUpDown)
    {
      infoPtr->currentYear=SendMessageW( infoPtr->hWndYearUpDown, UDM_SETPOS,   (WPARAM) 0,(LPARAM)0);
      if(!DestroyWindow(infoPtr->hWndYearUpDown))
	{
	  FIXME("Can't destroy Updown Control\n");
	}
      else
	infoPtr->hWndYearUpDown=0;
      if(!DestroyWindow(infoPtr->hWndYearEdit))
	{
	  FIXME("Can't destroy Updown Control\n");
	}
      else
	infoPtr->hWndYearEdit=0;
      InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
    }

  ht.pt.x = (short)LOWORD(lParam);
  ht.pt.y = (short)HIWORD(lParam);
  hit = MONTHCAL_HitTest(infoPtr, (LPARAM)&ht);

  /* FIXME: these flags should be checked by */
  /*((hit & MCHT_XXX) == MCHT_XXX) b/c some of the flags are */
  /* multi-bit */
  if(hit ==MCHT_TITLEBTNNEXT) {
    MONTHCAL_GoToNextMonth(infoPtr);
    infoPtr->status = MC_NEXTPRESSED;
    SetTimer(infoPtr->hwndSelf, MC_NEXTMONTHTIMER, MC_NEXTMONTHDELAY, 0);
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
    return 0;
  }
  if(hit == MCHT_TITLEBTNPREV){
    MONTHCAL_GoToPrevMonth(infoPtr);
    infoPtr->status = MC_PREVPRESSED;
    SetTimer(infoPtr->hwndSelf, MC_PREVMONTHTIMER, MC_NEXTMONTHDELAY, 0);
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
    return 0;
  }

  if(hit == MCHT_TITLEMONTH) {
    hMenu = CreatePopupMenu();

    for (i=0; i<12;i++)
      {
	GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_SMONTHNAME1+i, buf,countof(buf));
	AppendMenuW(hMenu, MF_STRING|MF_ENABLED,i+1, buf);
      }
    menupoint.x=infoPtr->titlemonth.right;
    menupoint.y=infoPtr->titlemonth.bottom;
    ClientToScreen(infoPtr->hwndSelf, &menupoint);
    i= TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_RETURNCMD,
		      menupoint.x, menupoint.y, 0, infoPtr->hwndSelf, NULL);
    if ((i>0) && (i<13))
      {
	infoPtr->currentMonth=i;
	InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
      }
  }
  if(hit == MCHT_TITLEYEAR) {
    infoPtr->hWndYearEdit=CreateWindowExW(0,
			 EditW,
			   0,
			 WS_VISIBLE | WS_CHILD |UDS_SETBUDDYINT,
			 infoPtr->titleyear.left+3,infoPtr->titlebtnnext.top,
			 infoPtr->titleyear.right-infoPtr->titleyear.left+4,
			 infoPtr->textHeight,
			 infoPtr->hwndSelf,
			 NULL,
			 NULL,
			 NULL);
    SendMessageW( infoPtr->hWndYearEdit, WM_SETFONT, (WPARAM) infoPtr->hBoldFont, (LPARAM)TRUE);
    infoPtr->hWndYearUpDown=CreateWindowExW(0,
			 UPDOWN_CLASSW,
			   0,
			 WS_VISIBLE | WS_CHILD |UDS_SETBUDDYINT|UDS_NOTHOUSANDS|UDS_ARROWKEYS,
			 infoPtr->titleyear.right+7,infoPtr->titlebtnnext.top,
			 18,
			 infoPtr->textHeight,
			 infoPtr->hwndSelf,
			 NULL,
			 NULL,
			 NULL);
    SendMessageW( infoPtr->hWndYearUpDown, UDM_SETRANGE, (WPARAM) 0, MAKELONG (9999, 1753));
    SendMessageW( infoPtr->hWndYearUpDown, UDM_SETBUDDY, (WPARAM) infoPtr->hWndYearEdit, (LPARAM)0 );
    SendMessageW( infoPtr->hWndYearUpDown, UDM_SETPOS,   (WPARAM) 0,(LPARAM)infoPtr->currentYear );
    return 0;

  }
  if(hit == MCHT_TODAYLINK) {
    NMSELCHANGE nmsc;

    infoPtr->curSelDay = infoPtr->todaysDate.wDay;
    infoPtr->firstSelDay = infoPtr->todaysDate.wDay;
    infoPtr->currentMonth=infoPtr->todaysDate.wMonth;
    infoPtr->currentYear=infoPtr->todaysDate.wYear;
    MONTHCAL_CopyTime(&infoPtr->todaysDate, &infoPtr->minSel);
    MONTHCAL_CopyTime(&infoPtr->todaysDate, &infoPtr->maxSel);
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

    nmsc.nmhdr.hwndFrom = infoPtr->hwndSelf;
    nmsc.nmhdr.idFrom   = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
    nmsc.nmhdr.code     = MCN_SELCHANGE;
    MONTHCAL_CopyTime(&infoPtr->minSel, &nmsc.stSelStart);
    MONTHCAL_CopyTime(&infoPtr->maxSel, &nmsc.stSelEnd);
    SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, (WPARAM)nmsc.nmhdr.idFrom, (LPARAM)&nmsc);

    nmsc.nmhdr.code     = MCN_SELECT;
    SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, (WPARAM)nmsc.nmhdr.idFrom,(LPARAM)&nmsc);
    return 0;
  }
  if(hit == MCHT_CALENDARDATE) {
    SYSTEMTIME selArray[2];
    NMSELCHANGE nmsc;

    MONTHCAL_CopyTime(&ht.st, &selArray[0]);
    MONTHCAL_CopyTime(&ht.st, &selArray[1]);
    MONTHCAL_SetSelRange(infoPtr, (LPARAM)&selArray);
    MONTHCAL_SetCurSel(infoPtr, (LPARAM)&selArray);
    TRACE("MCHT_CALENDARDATE\n");
    nmsc.nmhdr.hwndFrom = infoPtr->hwndSelf;
    nmsc.nmhdr.idFrom   = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
    nmsc.nmhdr.code     = MCN_SELCHANGE;
    MONTHCAL_CopyTime(&infoPtr->minSel,&nmsc.stSelStart);
    MONTHCAL_CopyTime(&infoPtr->maxSel,&nmsc.stSelEnd);

    SendMessageW(infoPtr->hwndNotify, WM_NOTIFY,
           (WPARAM)nmsc.nmhdr.idFrom,(LPARAM)&nmsc);


    /* redraw both old and new days if the selected day changed */
    if(infoPtr->curSelDay != ht.st.wDay) {
      MONTHCAL_CalcPosFromDay(infoPtr, ht.st.wDay, ht.st.wMonth, &rcDay);
      InvalidateRect(infoPtr->hwndSelf, &rcDay, TRUE);

      MONTHCAL_CalcPosFromDay(infoPtr, infoPtr->curSelDay, infoPtr->currentMonth, &rcDay);
      InvalidateRect(infoPtr->hwndSelf, &rcDay, TRUE);
    }

    infoPtr->firstSelDay = ht.st.wDay;
    infoPtr->curSelDay = ht.st.wDay;
    infoPtr->status = MC_SEL_LBUTDOWN;
    return 0;
  }

  return 1;
}


static LRESULT
MONTHCAL_LButtonUp(MONTHCAL_INFO *infoPtr, LPARAM lParam)
{
  NMSELCHANGE nmsc;
  NMHDR nmhdr;
  BOOL redraw = FALSE;
  MCHITTESTINFO ht;
  DWORD hit;

  TRACE("\n");

  if(infoPtr->status & MC_NEXTPRESSED) {
    KillTimer(infoPtr->hwndSelf, MC_NEXTMONTHTIMER);
    infoPtr->status &= ~MC_NEXTPRESSED;
    redraw = TRUE;
  }
  if(infoPtr->status & MC_PREVPRESSED) {
    KillTimer(infoPtr->hwndSelf, MC_PREVMONTHTIMER);
    infoPtr->status &= ~MC_PREVPRESSED;
    redraw = TRUE;
  }

  ht.pt.x = (short)LOWORD(lParam);
  ht.pt.y = (short)HIWORD(lParam);
  hit = MONTHCAL_HitTest(infoPtr, (LPARAM)&ht);

  infoPtr->status = MC_SEL_LBUTUP;

  if(hit ==MCHT_CALENDARDATENEXT) {
    MONTHCAL_GoToNextMonth(infoPtr);
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
    return TRUE;
  }
  if(hit == MCHT_CALENDARDATEPREV){
    MONTHCAL_GoToPrevMonth(infoPtr);
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
    return TRUE;
  }
  nmhdr.hwndFrom = infoPtr->hwndSelf;
  nmhdr.idFrom   = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
  nmhdr.code     = NM_RELEASEDCAPTURE;
  TRACE("Sent notification from %p to %p\n", infoPtr->hwndSelf, infoPtr->hwndNotify);

  SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);
  /* redraw if necessary */
  if(redraw)
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
  /* only send MCN_SELECT if currently displayed month's day was selected */
  if(hit == MCHT_CALENDARDATE) {
    nmsc.nmhdr.hwndFrom = infoPtr->hwndSelf;
    nmsc.nmhdr.idFrom   = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
    nmsc.nmhdr.code     = MCN_SELECT;
    MONTHCAL_CopyTime(&infoPtr->minSel, &nmsc.stSelStart);
    MONTHCAL_CopyTime(&infoPtr->maxSel, &nmsc.stSelEnd);

    SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, (WPARAM)nmsc.nmhdr.idFrom, (LPARAM)&nmsc);

  }
  return 0;
}


static LRESULT
MONTHCAL_Timer(MONTHCAL_INFO *infoPtr, WPARAM wParam)
{
  BOOL redraw = FALSE;

  TRACE("%ld\n", wParam);

  switch(wParam) {
  case MC_NEXTMONTHTIMER:
    redraw = TRUE;
    MONTHCAL_GoToNextMonth(infoPtr);
    break;
  case MC_PREVMONTHTIMER:
    redraw = TRUE;
    MONTHCAL_GoToPrevMonth(infoPtr);
    break;
  default:
    ERR("got unknown timer\n");
    break;
  }

  /* redraw only if necessary */
  if(redraw)
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

  return 0;
}


static LRESULT
MONTHCAL_MouseMove(MONTHCAL_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
  MCHITTESTINFO ht;
  int oldselday, selday, hit;
  RECT r;

  if(!(infoPtr->status & MC_SEL_LBUTDOWN)) return 0;

  ht.pt.x = (short)LOWORD(lParam);
  ht.pt.y = (short)HIWORD(lParam);

  hit = MONTHCAL_HitTest(infoPtr, (LPARAM)&ht);

  /* not on the calendar date numbers? bail out */
  TRACE("hit:%x\n",hit);
  if((hit & MCHT_CALENDARDATE) != MCHT_CALENDARDATE) return 0;

  selday = ht.st.wDay;
  oldselday = infoPtr->curSelDay;
  infoPtr->curSelDay = selday;
  MONTHCAL_CalcPosFromDay(infoPtr, selday, ht.st. wMonth, &r);

  if(GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE) & MCS_MULTISELECT)  {
    SYSTEMTIME selArray[2];
    int i;

    MONTHCAL_GetSelRange(infoPtr, (LPARAM)&selArray);
    i = 0;
    if(infoPtr->firstSelDay==selArray[0].wDay) i=1;
    TRACE("oldRange:%d %d %d %d\n", infoPtr->firstSelDay, selArray[0].wDay, selArray[1].wDay, i);
    if(infoPtr->firstSelDay==selArray[1].wDay) {
      /* 1st time we get here: selArray[0]=selArray[1])  */
      /* if we're still at the first selected date, return */
      if(infoPtr->firstSelDay==selday) goto done;
      if(selday<infoPtr->firstSelDay) i = 0;
    }

    if(abs(infoPtr->firstSelDay - selday) >= infoPtr->maxSelCount) {
      if(selday>infoPtr->firstSelDay)
        selday = infoPtr->firstSelDay + infoPtr->maxSelCount;
      else
        selday = infoPtr->firstSelDay - infoPtr->maxSelCount;
    }

    if(selArray[i].wDay!=selday) {
      TRACE("newRange:%d %d %d %d\n", infoPtr->firstSelDay, selArray[0].wDay, selArray[1].wDay, i);

      selArray[i].wDay = selday;

      if(selArray[0].wDay>selArray[1].wDay) {
        DWORD tempday;
        tempday = selArray[1].wDay;
        selArray[1].wDay = selArray[0].wDay;
        selArray[0].wDay = tempday;
      }

      MONTHCAL_SetSelRange(infoPtr, (LPARAM)&selArray);
    }
  }

done:

  /* only redraw if the currently selected day changed */
  /* FIXME: this should specify a rectangle containing only the days that changed */
  /* using InvalidateRect */
  if(oldselday != infoPtr->curSelDay)
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

  return 0;
}


static LRESULT
MONTHCAL_Paint(MONTHCAL_INFO *infoPtr, WPARAM wParam)
{
  HDC hdc;
  PAINTSTRUCT ps;

  if (wParam)
  {
    GetClientRect(infoPtr->hwndSelf, &ps.rcPaint);
    hdc = (HDC)wParam;
  }
  else
    hdc = BeginPaint(infoPtr->hwndSelf, &ps);

  MONTHCAL_Refresh(infoPtr, hdc, &ps);
  if (!wParam) EndPaint(infoPtr->hwndSelf, &ps);
  return 0;
}


static LRESULT
MONTHCAL_KillFocus(const MONTHCAL_INFO *infoPtr, HWND hFocusWnd)
{
  TRACE("\n");

  if (infoPtr->hwndNotify != hFocusWnd)
    ShowWindow(infoPtr->hwndSelf, SW_HIDE);
  else
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

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
  static const WCHAR SunW[] = { 'S','u','n',0 };
  static const WCHAR O0W[] = { '0','0',0 };
  HDC hdc = GetDC(infoPtr->hwndSelf);
  RECT *title=&infoPtr->title;
  RECT *prev=&infoPtr->titlebtnprev;
  RECT *next=&infoPtr->titlebtnnext;
  RECT *titlemonth=&infoPtr->titlemonth;
  RECT *titleyear=&infoPtr->titleyear;
  RECT *wdays=&infoPtr->wdays;
  RECT *weeknumrect=&infoPtr->weeknums;
  RECT *days=&infoPtr->days;
  RECT *todayrect=&infoPtr->todayrect;
  SIZE size;
  TEXTMETRICW tm;
  DWORD dwStyle = GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE);
  HFONT currentFont;
  int xdiv, left_offset;
  RECT rcClient;

  GetClientRect(infoPtr->hwndSelf, &rcClient);

  currentFont = SelectObject(hdc, infoPtr->hFont);

  /* get the height and width of each day's text */
  GetTextMetricsW(hdc, &tm);
  infoPtr->textHeight = tm.tmHeight + tm.tmExternalLeading + tm.tmInternalLeading;
  GetTextExtentPoint32W(hdc, SunW, 3, &size);
  infoPtr->textWidth = size.cx + 2;

  /* recalculate the height and width increments and offsets */
  GetTextExtentPoint32W(hdc, O0W, 2, &size);

  xdiv = (dwStyle & MCS_WEEKNUMBERS) ? 8 : 7;

  infoPtr->width_increment = size.cx * 2 + 4;
  infoPtr->height_increment = infoPtr->textHeight;
  left_offset = (rcClient.right - rcClient.left) - (infoPtr->width_increment * xdiv);

  /* calculate title area */
  title->top    = rcClient.top;
  title->bottom = title->top + 3 * infoPtr->height_increment / 2;
  title->left   = left_offset;
  title->right  = rcClient.right;

  /* set the dimensions of the next and previous buttons and center */
  /* the month text vertically */
  prev->top    = next->top    = title->top + 4;
  prev->bottom = next->bottom = title->bottom - 4;
  prev->left   = title->left + 4;
  prev->right  = prev->left + (title->bottom - title->top) ;
  next->right  = title->right - 4;
  next->left   = next->right - (title->bottom - title->top);

  /* titlemonth->left and right change based upon the current month */
  /* and are recalculated in refresh as the current month may change */
  /* without the control being resized */
  titlemonth->top    = titleyear->top    = title->top    + (infoPtr->height_increment)/2;
  titlemonth->bottom = titleyear->bottom = title->bottom - (infoPtr->height_increment)/2;

  /* setup the dimensions of the rectangle we draw the names of the */
  /* days of the week in */
  weeknumrect->left = left_offset;
  if(dwStyle & MCS_WEEKNUMBERS)
    weeknumrect->right=prev->right;
  else
    weeknumrect->right=weeknumrect->left;
  wdays->left   = days->left   = weeknumrect->right;
  wdays->right  = days->right  = wdays->left + 7 * infoPtr->width_increment;
  wdays->top    = title->bottom ;
  wdays->bottom = wdays->top + infoPtr->height_increment;

  days->top    = weeknumrect->top = wdays->bottom ;
  days->bottom = weeknumrect->bottom = days->top + 6 * infoPtr->height_increment;

  todayrect->left   = rcClient.left;
  todayrect->right  = rcClient.right;
  todayrect->top    = days->bottom;
  todayrect->bottom = days->bottom + infoPtr->height_increment;

  TRACE("dx=%d dy=%d client[%s] title[%s] wdays[%s] days[%s] today[%s]\n",
	infoPtr->width_increment,infoPtr->height_increment,
        wine_dbgstr_rect(&rcClient),
        wine_dbgstr_rect(title),
        wine_dbgstr_rect(wdays),
        wine_dbgstr_rect(days),
        wine_dbgstr_rect(todayrect));

  /* restore the originally selected font */
  SelectObject(hdc, currentFont);

  ReleaseDC(infoPtr->hwndSelf, hdc);
}

static LRESULT MONTHCAL_Size(MONTHCAL_INFO *infoPtr, int Width, int Height)
{
  TRACE("(width=%d, height=%d)\n", Width, Height);

  MONTHCAL_UpdateSize(infoPtr);

  /* invalidate client area and erase background */
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

    if (redraw)
        InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

    return (LRESULT)hOldFont;
}

/* update theme after a WM_THEMECHANGED message */
static LRESULT theme_changed (const MONTHCAL_INFO* infoPtr)
{
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
    CloseThemeData (theme);
    theme = OpenThemeData (infoPtr->hwndSelf, themeClass);
    return 0;
}

/* FIXME: check whether dateMin/dateMax need to be adjusted. */
static LRESULT
MONTHCAL_Create(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr;

  /* allocate memory for info structure */
  infoPtr =(MONTHCAL_INFO*)Alloc(sizeof(MONTHCAL_INFO));
  SetWindowLongPtrW(hwnd, 0, (DWORD_PTR)infoPtr);

  if(infoPtr == NULL) {
    ERR( "could not allocate info memory!\n");
    return 0;
  }

  infoPtr->hwndSelf = hwnd;
  infoPtr->hwndNotify = ((LPCREATESTRUCTW)lParam)->hwndParent;

  MONTHCAL_SetFont(infoPtr, GetStockObject(DEFAULT_GUI_FONT), FALSE);

  /* initialize info structure */
  /* FIXME: calculate systemtime ->> localtime(substract timezoneinfo) */

  GetLocalTime(&infoPtr->todaysDate);
  infoPtr->firstDayHighWord = FALSE;
  MONTHCAL_SetFirstDayOfWeek(infoPtr, (LPARAM)-1);
  infoPtr->currentMonth = infoPtr->todaysDate.wMonth;
  infoPtr->currentYear = infoPtr->todaysDate.wYear;
  MONTHCAL_CopyTime(&infoPtr->todaysDate, &infoPtr->minDate);
  MONTHCAL_CopyTime(&infoPtr->todaysDate, &infoPtr->maxDate);
  infoPtr->maxDate.wYear=2050;
  infoPtr->minDate.wYear=1950;
  infoPtr->maxSelCount  = 7;
  infoPtr->monthRange = 3;
  infoPtr->monthdayState = Alloc
                         (infoPtr->monthRange * sizeof(MONTHDAYSTATE));
  infoPtr->titlebk     = GetSysColor(COLOR_ACTIVECAPTION);
  infoPtr->titletxt    = GetSysColor(COLOR_WINDOW);
  infoPtr->monthbk     = GetSysColor(COLOR_WINDOW);
  infoPtr->trailingtxt = GetSysColor(COLOR_GRAYTEXT);
  infoPtr->bk          = GetSysColor(COLOR_WINDOW);
  infoPtr->txt	       = GetSysColor(COLOR_WINDOWTEXT);

  /* set the current day for highlighing */
  infoPtr->minSel.wDay = infoPtr->todaysDate.wDay;
  infoPtr->maxSel.wDay = infoPtr->todaysDate.wDay;

  /* call MONTHCAL_UpdateSize to set all of the dimensions */
  /* of the control */
  MONTHCAL_UpdateSize(infoPtr);
  
  OpenThemeData (infoPtr->hwndSelf, themeClass);

  return 0;
}


static LRESULT
MONTHCAL_Destroy(MONTHCAL_INFO *infoPtr)
{
  /* free month calendar info data */
  Free(infoPtr->monthdayState);
  SetWindowLongPtrW(infoPtr->hwndSelf, 0, 0);

  CloseThemeData (GetWindowTheme (infoPtr->hwndSelf));
  
  Free(infoPtr);
  return 0;
}


static LRESULT WINAPI
MONTHCAL_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr;

  TRACE("hwnd=%p msg=%x wparam=%lx lparam=%lx\n", hwnd, uMsg, wParam, lParam);

  infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  if (!infoPtr && (uMsg != WM_CREATE))
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
  switch(uMsg)
  {
  case MCM_GETCURSEL:
    return MONTHCAL_GetCurSel(infoPtr, lParam);

  case MCM_SETCURSEL:
    return MONTHCAL_SetCurSel(infoPtr, lParam);

  case MCM_GETMAXSELCOUNT:
    return MONTHCAL_GetMaxSelCount(infoPtr);

  case MCM_SETMAXSELCOUNT:
    return MONTHCAL_SetMaxSelCount(infoPtr, wParam);

  case MCM_GETSELRANGE:
    return MONTHCAL_GetSelRange(infoPtr, lParam);

  case MCM_SETSELRANGE:
    return MONTHCAL_SetSelRange(infoPtr, lParam);

  case MCM_GETMONTHRANGE:
    return MONTHCAL_GetMonthRange(infoPtr);

  case MCM_SETDAYSTATE:
    return MONTHCAL_SetDayState(infoPtr, wParam, lParam);

  case MCM_GETMINREQRECT:
    return MONTHCAL_GetMinReqRect(infoPtr, lParam);

  case MCM_GETCOLOR:
    return MONTHCAL_GetColor(infoPtr, wParam);

  case MCM_SETCOLOR:
    return MONTHCAL_SetColor(infoPtr, wParam, lParam);

  case MCM_GETTODAY:
    return MONTHCAL_GetToday(infoPtr, lParam);

  case MCM_SETTODAY:
    return MONTHCAL_SetToday(infoPtr, lParam);

  case MCM_HITTEST:
    return MONTHCAL_HitTest(infoPtr, lParam);

  case MCM_GETFIRSTDAYOFWEEK:
    return MONTHCAL_GetFirstDayOfWeek(infoPtr);

  case MCM_SETFIRSTDAYOFWEEK:
    return MONTHCAL_SetFirstDayOfWeek(infoPtr, lParam);

  case MCM_GETRANGE:
    return MONTHCAL_GetRange(hwnd, wParam, lParam);

  case MCM_SETRANGE:
    return MONTHCAL_SetRange(infoPtr, wParam, lParam);

  case MCM_GETMONTHDELTA:
    return MONTHCAL_GetMonthDelta(infoPtr);

  case MCM_SETMONTHDELTA:
    return MONTHCAL_SetMonthDelta(infoPtr, wParam);

  case MCM_GETMAXTODAYWIDTH:
    return MONTHCAL_GetMaxTodayWidth(infoPtr);

  case WM_GETDLGCODE:
    return DLGC_WANTARROWS | DLGC_WANTCHARS;

  case WM_KILLFOCUS:
    return MONTHCAL_KillFocus(infoPtr, (HWND)wParam);

  case WM_RBUTTONDOWN:
    return MONTHCAL_RButtonDown(infoPtr, lParam);

  case WM_LBUTTONDOWN:
    return MONTHCAL_LButtonDown(infoPtr, lParam);

  case WM_MOUSEMOVE:
    return MONTHCAL_MouseMove(infoPtr, wParam, lParam);

  case WM_LBUTTONUP:
    return MONTHCAL_LButtonUp(infoPtr, lParam);

  case WM_PRINTCLIENT:
  case WM_PAINT:
    return MONTHCAL_Paint(infoPtr, wParam);

  case WM_SETFOCUS:
    return MONTHCAL_SetFocus(infoPtr);

  case WM_SIZE:
    return MONTHCAL_Size(infoPtr, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

  case WM_CREATE:
    return MONTHCAL_Create(hwnd, wParam, lParam);

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

  default:
    if ((uMsg >= WM_USER) && (uMsg < WM_APP))
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
