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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * TODO:
 *   - Notifications.
 *
 *
 *  FIXME: handle resources better (doesn't work now); also take care
           of internationalization.
 *  FIXME: keyboard handling.
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

typedef struct
{
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
    int		left_offset;
    int		top_offset;
    int		firstDayplace; /* place of the first day of the current month */
    int		delta;	/* scroll rate; # of months that the */
                        /* control moves when user clicks a scroll button */
    int		visible;	/* # of months visible */
    int		firstDay;	/* Start month calendar with firstDay's day */
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

    RECT rcClient;	/* rect for whole client area */
    RECT rcDraw;	/* rect for drawable portion of client area */
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


/* Offsets of days in the week to the weekday of  january 1. */
static const int DayOfWeekTable[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};


#define MONTHCAL_GetInfoPtr(hwnd) ((MONTHCAL_INFO *)GetWindowLongA(hwnd, 0))

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
  if(time.wMonth > 12) return FALSE;
  if(time.wDayOfWeek > 6) return FALSE;
  if(time.wDay > MONTHCAL_MonthLength(time.wMonth, time.wYear))
	  return FALSE;
  if(time.wHour > 23) return FALSE;
  if(time.wMinute > 59) return FALSE;
  if(time.wSecond > 59) return FALSE;
  if(time.wMilliseconds > 999) return FALSE;

  return TRUE;
}


void MONTHCAL_CopyTime(const SYSTEMTIME *from, SYSTEMTIME *to)
{
  to->wYear = from->wYear;
  to->wMonth = from->wMonth;
  to->wDayOfWeek = from->wDayOfWeek;
  to->wDay = from->wDay;
  to->wHour = from->wHour;
  to->wMinute = from->wMinute;
  to->wSecond = from->wSecond;
  to->wMilliseconds = from->wMilliseconds;
}


/* Note:Depending on DST, this may be offset by a day.
   Need to find out if we're on a DST place & adjust the clock accordingly.
   Above function assumes we have a valid data.
   Valid for year>1752;  1 <= d <= 31, 1 <= m <= 12.
   0 = Monday.
*/

/* returns the day in the week(0 == monday, 6 == sunday) */
/* day(1 == 1st, 2 == 2nd... etc), year is the  year value */
static int MONTHCAL_CalculateDayOfWeek(DWORD day, DWORD month, DWORD year)
{
  year-=(month < 3);

  return((year + year/4 - year/100 + year/400 +
         DayOfWeekTable[month-1] + day - 1 ) % 7);
}

/* From a given point, calculate the row (weekpos), column(daypos)
   and day in the calendar. day== 0 mean the last day of tha last month
*/
static int MONTHCAL_CalcDayFromPos(MONTHCAL_INFO *infoPtr, int x, int y,
				   int *daypos,int *weekpos)
{
  int retval, firstDay;

  /* if the point is outside the x bounds of the window put
  it at the boundry */
  if(x > infoPtr->rcClient.right) {
    x = infoPtr->rcClient.right ;
  }

  *daypos = (x - infoPtr->days.left ) / infoPtr->width_increment;
  *weekpos = (y - infoPtr->days.top ) / infoPtr->height_increment;

  firstDay = (MONTHCAL_CalculateDayOfWeek(1, infoPtr->currentMonth, infoPtr->currentYear)+6 - infoPtr->firstDay)%7;
  retval = *daypos + (7 * *weekpos) - firstDay;
  return retval;
}

/* day is the day of the month, 1 == 1st day of the month */
/* sets x and y to be the position of the day */
/* x == day, y == week where(0,0) == firstDay, 1st week */
static void MONTHCAL_CalcDayXY(MONTHCAL_INFO *infoPtr, int day, int month,
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
static void MONTHCAL_CalcDayRect(MONTHCAL_INFO *infoPtr, RECT *r, int x, int y)
{
  r->left = infoPtr->days.left + x * infoPtr->width_increment;
  r->right = r->left + infoPtr->width_increment;
  r->top  = infoPtr->days.top  + y * infoPtr->height_increment;
  r->bottom = r->top + infoPtr->textHeight;
}


/* sets the RECT struct r to the rectangle around the day and month */
/* day is the day value of the month(1 == 1st), month is the month */
/* value(january == 1, december == 12) */
static inline void MONTHCAL_CalcPosFromDay(MONTHCAL_INFO *infoPtr,
                                            int day, int month, RECT *r)
{
  int x, y;

  MONTHCAL_CalcDayXY(infoPtr, day, month, &x, &y);
  MONTHCAL_CalcDayRect(infoPtr, r, x, y);
}


/* day is the day in the month(1 == 1st of the month) */
/* month is the month value(1 == january, 12 == december) */
static void MONTHCAL_CircleDay(HDC hdc, MONTHCAL_INFO *infoPtr, int day,
int month)
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


static void MONTHCAL_DrawDay(HDC hdc, MONTHCAL_INFO *infoPtr, int day, int month,
                             int x, int y, int bold)
{
  char buf[10];
  RECT r;
  static int haveBoldFont, haveSelectedDay = FALSE;
  HBRUSH hbr;
  HPEN hNewPen, hOldPen = 0;
  COLORREF oldCol = 0;
  COLORREF oldBk = 0;

  sprintf(buf, "%d", day);

/* No need to check styles: when selection is not valid, it is set to zero.
 * 1<day<31, so evertyhing's OK.
 */

  MONTHCAL_CalcDayRect(infoPtr, &r, x, y);

  if((day>=infoPtr->minSel.wDay) && (day<=infoPtr->maxSel.wDay)
       && (month==infoPtr->currentMonth)) {
    HRGN hrgn;
    RECT r2;

    TRACE("%d %d %d\n",day, infoPtr->minSel.wDay, infoPtr->maxSel.wDay);
    TRACE("%ld %ld %ld %ld\n", r.left, r.top, r.right, r.bottom);
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
  DrawTextA(hdc, buf, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE );

  /* draw a rectangle around the currently selected days text */
  if((day==infoPtr->curSelDay) && (month==infoPtr->currentMonth)) {
    hNewPen = CreatePen(PS_ALTERNATE, 0, GetSysColor(COLOR_WINDOWTEXT) );
    hbr = GetSysColorBrush(COLOR_WINDOWTEXT);
    FrameRect(hdc, &r, hbr);
    SelectObject(hdc, hOldPen);
  }
}


/* CHECKME: For `todays date', do we need to check the locale?*/
static void MONTHCAL_Refresh(HWND hwnd, HDC hdc, PAINTSTRUCT* ps)
{
  MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
  RECT *rcClient=&infoPtr->rcClient;
  RECT *rcDraw=&infoPtr->rcDraw;
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
  /* LOGFONTA logFont; */
  char buf[20];
  char buf1[20];
  char buf2[32];
  COLORREF oldTextColor, oldBkColor;
  DWORD dwStyle = GetWindowLongA(hwnd, GWL_STYLE);
  RECT rcTemp;
  RECT rcDay; /* used in MONTHCAL_CalcDayRect() */
  SYSTEMTIME localtime;
  int startofprescal;

  oldTextColor = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));


  /* fill background */
  hbr = CreateSolidBrush (infoPtr->bk);
  FillRect(hdc, rcClient, hbr);
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
  {
    if((infoPtr->status & MC_PREVPRESSED))
        DrawFrameControl(hdc, prev, DFC_SCROLL,
  	   DFCS_SCROLLLEFT | DFCS_PUSHED |
          (dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0));
    else /* if the previous button is pressed draw it depressed */
      DrawFrameControl(hdc, prev, DFC_SCROLL,
	   DFCS_SCROLLLEFT |(dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0));
  }

  /* if next button is depressed draw it depressed */
  if(IntersectRect(&rcTemp, &(ps->rcPaint), next))
  {
    if((infoPtr->status & MC_NEXTPRESSED))
      DrawFrameControl(hdc, next, DFC_SCROLL,
    	   DFCS_SCROLLRIGHT | DFCS_PUSHED |
           (dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0));
    else /* if the next button is pressed draw it depressed */
      DrawFrameControl(hdc, next, DFC_SCROLL,
           DFCS_SCROLLRIGHT |(dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0));
  }

  oldBkColor = SetBkColor(hdc, infoPtr->titlebk);
  SetTextColor(hdc, infoPtr->titletxt);
  currentFont = SelectObject(hdc, infoPtr->hBoldFont);

  /* titlemonth->left and right are set in MONTHCAL_UpdateSize */
  titlemonth->left   = title->left;
  titlemonth->right  = title->right;

  GetLocaleInfoA( LOCALE_USER_DEFAULT,LOCALE_SMONTHNAME1+infoPtr->currentMonth -1,
		  buf1,sizeof(buf1));
  sprintf(buf, "%s %ld", buf1, infoPtr->currentYear);

  if(IntersectRect(&rcTemp, &(ps->rcPaint), titlemonth))
  {
    DrawTextA(hdc, buf, strlen(buf), titlemonth,
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  }

  SelectObject(hdc, infoPtr->hFont);

/* titlemonth left/right contained rect for whole titletxt('June  1999')
  * MCM_HitTestInfo wants month & year rects, so prepare these now.
  *(no, we can't draw them separately; the whole text is centered)
  */
  GetTextExtentPoint32A(hdc, buf, strlen(buf), &size);
  titlemonth->left = title->right / 2 - size.cx / 2;
  titleyear->right = title->right / 2 + size.cx / 2;
  GetTextExtentPoint32A(hdc, buf1, strlen(buf1), &size);
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

  LineTo(hdc, rcDraw->right - 3, title->bottom + textHeight + 1);

  prevMonth = infoPtr->currentMonth - 1;
  if(prevMonth == 0) /* if currentMonth is january(1) prevMonth is */
    prevMonth = 12;    /* december(12) of the previous year */

  infoPtr->wdays.left   = infoPtr->days.left   = infoPtr->weeknums.right;
/* draw day abbreviations */

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
    GetLocaleInfoA( LOCALE_USER_DEFAULT,LOCALE_SABBREVDAYNAME1 + (i +j)%7,
		    buf,sizeof(buf));
    DrawTextA(hdc, buf, strlen(buf), days,
                         DT_CENTER | DT_VCENTER | DT_SINGLELINE );
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
      MONTHCAL_DrawDay(hdc, infoPtr, day, prevMonth, i, 0,
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

      MONTHCAL_DrawDay(hdc, infoPtr, day, infoPtr->currentMonth, i, 0,
	infoPtr->monthdayState[m] & mask);

      if((infoPtr->currentMonth==infoPtr->todaysDate.wMonth) &&
          (day==infoPtr->todaysDate.wDay) &&
	  (infoPtr->currentYear == infoPtr->todaysDate.wYear)) {
        if(!(dwStyle & MCS_NOTODAYCIRCLE))
	  MONTHCAL_CircleDay(hdc, infoPtr, day, infoPtr->currentMonth);
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
      MONTHCAL_DrawDay(hdc, infoPtr, day, infoPtr->currentMonth, i, j,
          infoPtr->monthdayState[m] & mask);

      if((infoPtr->currentMonth==infoPtr->todaysDate.wMonth) &&
          (day==infoPtr->todaysDate.wDay) &&
          (infoPtr->currentYear == infoPtr->todaysDate.wYear))
        if(!(dwStyle & MCS_NOTODAYCIRCLE))
	  MONTHCAL_CircleDay(hdc, infoPtr, day, infoPtr->currentMonth);
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
      MONTHCAL_DrawDay(hdc, infoPtr, day, infoPtr->currentMonth + 1, i, j,
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
      MONTHCAL_CircleDay(hdc, infoPtr,
			 day+MONTHCAL_MonthLength(infoPtr->currentMonth,infoPtr->currentYear),
			 infoPtr->currentMonth);
      offset+=textWidth;
    }
    if (!LoadStringA(COMCTL32_hModule,IDM_TODAY,buf1,sizeof(buf1)))
      {
	WARN("Can't load resource\n");
	strcpy(buf1,"Today:");
      }
    MONTHCAL_CalcDayRect(infoPtr, &rtoday, 1, 6);
    MONTHCAL_CopyTime(&infoPtr->todaysDate,&localtime);
    GetDateFormatA(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&localtime,NULL,buf2,sizeof(buf2));
    sprintf(buf, "%s %s", buf1,buf2);
    SelectObject(hdc, infoPtr->hBoldFont);

    if(IntersectRect(&rcTemp, &(ps->rcPaint), &rtoday))
    {
      DrawTextA(hdc, buf, -1, &rtoday, DT_CALCRECT | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      DrawTextA(hdc, buf, -1, &rtoday, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
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
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IFIRSTWEEKOFYEAR,
		     buf, sizeof(buf));
    sscanf(buf, "%d", &weeknum);
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
	  sprintf(buf, "%d", weeknum);
	  weeknum=0;
	}
      else if((i==5)&&(weeknum>47))
	{
	  sprintf(buf, "%d", 1);
	}
      else
	sprintf(buf, "%d", weeknum + i);
      DrawTextA(hdc, buf, -1, days, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
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
MONTHCAL_GetMinReqRect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  LPRECT lpRect = (LPRECT) lParam;
  TRACE("%x %lx\n", wParam, lParam);

  /* validate parameters */

  if((infoPtr==NULL) ||(lpRect == NULL) ) return FALSE;

  lpRect->left = infoPtr->rcClient.left;
  lpRect->right = infoPtr->rcClient.right;
  lpRect->top = infoPtr->rcClient.top;
  lpRect->bottom = infoPtr->rcClient.bottom;
  return TRUE;
}


static LRESULT
MONTHCAL_GetColor(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  TRACE("%x %lx\n", wParam, lParam);

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
MONTHCAL_SetColor(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  int prev = -1;

  TRACE("%x %lx\n", wParam, lParam);

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

  InvalidateRect(hwnd, NULL, FALSE);
  return prev;
}


static LRESULT
MONTHCAL_GetMonthDelta(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  TRACE("%x %lx\n", wParam, lParam);

  if(infoPtr->delta)
    return infoPtr->delta;
  else
    return infoPtr->visible;
}


static LRESULT
MONTHCAL_SetMonthDelta(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  int prev = infoPtr->delta;

  TRACE("%x %lx\n", wParam, lParam);

  infoPtr->delta = (int)wParam;
  return prev;
}


static LRESULT
MONTHCAL_GetFirstDayOfWeek(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  return infoPtr->firstDay;
}


/* sets the first day of the week that will appear in the control */
/* 0 == Monday, 6 == Sunday */
/* FIXME: this needs to be implemented properly in MONTHCAL_Refresh() */
/* FIXME: we need more error checking here */
static LRESULT
MONTHCAL_SetFirstDayOfWeek(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  int prev = infoPtr->firstDay;
  char buf[40];
  int day;

  TRACE("%x %lx\n", wParam, lParam);

  if((lParam >= 0) && (lParam < 7)) {
    infoPtr->firstDay = (int)lParam;
  }
  else
    {
      GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK,
		     buf, sizeof(buf));
      TRACE("%s %d\n", buf, strlen(buf));
      if(sscanf(buf, "%d", &day) == 1)
	infoPtr->firstDay = day;
      else
	infoPtr->firstDay = 0;
    }
  return prev;
}


/* FIXME: fill this in */
static LRESULT
MONTHCAL_GetMonthRange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  TRACE("%x %lx\n", wParam, lParam);
  FIXME("stub\n");

  return infoPtr->monthRange;
}


static LRESULT
MONTHCAL_GetMaxTodayWidth(HWND hwnd)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  return(infoPtr->todayrect.right - infoPtr->todayrect.left);
}


/* FIXME: are validated times taken from current date/time or simply
 * copied?
 * FIXME:    check whether MCM_GETMONTHRANGE shows correct result after
 *            adjusting range with MCM_SETRANGE
 */

static LRESULT
MONTHCAL_SetRange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lprgSysTimeArray=(SYSTEMTIME *)lParam;
  int prev;

  TRACE("%x %lx\n", wParam, lParam);

  if(wParam & GDTR_MAX) {
    if(MONTHCAL_ValidateTime(lprgSysTimeArray[1])){
      MONTHCAL_CopyTime(&lprgSysTimeArray[1], &infoPtr->maxDate);
      infoPtr->rangeValid|=GDTR_MAX;
    } else  {
      GetSystemTime(&infoPtr->todaysDate);
      MONTHCAL_CopyTime(&infoPtr->todaysDate, &infoPtr->maxDate);
    }
  }
  if(wParam & GDTR_MIN) {
    if(MONTHCAL_ValidateTime(lprgSysTimeArray[0])) {
      MONTHCAL_CopyTime(&lprgSysTimeArray[0], &infoPtr->maxDate);
      infoPtr->rangeValid|=GDTR_MIN;
    } else {
      GetSystemTime(&infoPtr->todaysDate);
      MONTHCAL_CopyTime(&infoPtr->todaysDate, &infoPtr->maxDate);
    }
  }

  prev = infoPtr->monthRange;
  infoPtr->monthRange = infoPtr->maxDate.wMonth - infoPtr->minDate.wMonth;

  if(infoPtr->monthRange!=prev) {
	infoPtr->monthdayState = ReAlloc(infoPtr->monthdayState,
                                                  infoPtr->monthRange * sizeof(MONTHDAYSTATE));
  }

  return 1;
}


/* CHECKME: At the moment, we copy ranges anyway,regardless of
 * infoPtr->rangeValid; a invalid range is simply filled with zeros in
 * SetRange.  Is this the right behavior?
*/

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
MONTHCAL_SetDayState(HWND hwnd, WPARAM wParam, LPARAM lParam)

{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  int i, iMonths = (int)wParam;
  MONTHDAYSTATE *dayStates = (LPMONTHDAYSTATE)lParam;

  TRACE("%x %lx\n", wParam, lParam);
  if(iMonths!=infoPtr->monthRange) return 0;

  for(i=0; i<iMonths; i++)
    infoPtr->monthdayState[i] = dayStates[i];
  return 1;
}

static LRESULT
MONTHCAL_GetCurSel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lpSel = (SYSTEMTIME *) lParam;

  TRACE("%x %lx\n", wParam, lParam);
  if((infoPtr==NULL) ||(lpSel==NULL)) return FALSE;
  if(GetWindowLongA(hwnd, GWL_STYLE) & MCS_MULTISELECT) return FALSE;

  MONTHCAL_CopyTime(&infoPtr->minSel, lpSel);
  TRACE("%d/%d/%d\n", lpSel->wYear, lpSel->wMonth, lpSel->wDay);
  return TRUE;
}

/* FIXME: if the specified date is not visible, make it visible */
/* FIXME: redraw? */
static LRESULT
MONTHCAL_SetCurSel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lpSel = (SYSTEMTIME *)lParam;

  TRACE("%x %lx\n", wParam, lParam);
  if((infoPtr==NULL) ||(lpSel==NULL)) return FALSE;
  if(GetWindowLongA(hwnd, GWL_STYLE) & MCS_MULTISELECT) return FALSE;

  infoPtr->currentMonth=lpSel->wMonth;
  infoPtr->currentYear=lpSel->wYear;

  MONTHCAL_CopyTime(lpSel, &infoPtr->minSel);
  MONTHCAL_CopyTime(lpSel, &infoPtr->maxSel);

  InvalidateRect(hwnd, NULL, FALSE);

  return TRUE;
}


static LRESULT
MONTHCAL_GetMaxSelCount(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  TRACE("%x %lx\n", wParam, lParam);
  return infoPtr->maxSelCount;
}


static LRESULT
MONTHCAL_SetMaxSelCount(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  TRACE("%x %lx\n", wParam, lParam);
  if(GetWindowLongA(hwnd, GWL_STYLE) & MCS_MULTISELECT)  {
    infoPtr->maxSelCount = wParam;
  }

  return TRUE;
}


static LRESULT
MONTHCAL_GetSelRange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lprgSysTimeArray = (SYSTEMTIME *) lParam;

  TRACE("%x %lx\n", wParam, lParam);

  /* validate parameters */

  if((infoPtr==NULL) ||(lprgSysTimeArray==NULL)) return FALSE;

  if(GetWindowLongA(hwnd, GWL_STYLE) & MCS_MULTISELECT)
  {
    MONTHCAL_CopyTime(&infoPtr->maxSel, &lprgSysTimeArray[1]);
    MONTHCAL_CopyTime(&infoPtr->minSel, &lprgSysTimeArray[0]);
    TRACE("[min,max]=[%d %d]\n", infoPtr->minSel.wDay, infoPtr->maxSel.wDay);
    return TRUE;
  }

  return FALSE;
}


static LRESULT
MONTHCAL_SetSelRange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lprgSysTimeArray = (SYSTEMTIME *) lParam;

  TRACE("%x %lx\n", wParam, lParam);

  /* validate parameters */

  if((infoPtr==NULL) ||(lprgSysTimeArray==NULL)) return FALSE;

  if(GetWindowLongA( hwnd, GWL_STYLE) & MCS_MULTISELECT)
  {
    MONTHCAL_CopyTime(&lprgSysTimeArray[1], &infoPtr->maxSel);
    MONTHCAL_CopyTime(&lprgSysTimeArray[0], &infoPtr->minSel);
    TRACE("[min,max]=[%d %d]\n", infoPtr->minSel.wDay, infoPtr->maxSel.wDay);
    return TRUE;
  }

  return FALSE;
}


static LRESULT
MONTHCAL_GetToday(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lpToday = (SYSTEMTIME *) lParam;

  TRACE("%x %lx\n", wParam, lParam);

  /* validate parameters */

  if((infoPtr==NULL) || (lpToday==NULL)) return FALSE;
  MONTHCAL_CopyTime(&infoPtr->todaysDate, lpToday);
  return TRUE;
}


static LRESULT
MONTHCAL_SetToday(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lpToday = (SYSTEMTIME *) lParam;

  TRACE("%x %lx\n", wParam, lParam);

  /* validate parameters */

  if((infoPtr==NULL) ||(lpToday==NULL)) return FALSE;
  MONTHCAL_CopyTime(lpToday, &infoPtr->todaysDate);
  InvalidateRect(hwnd, NULL, FALSE);
  return TRUE;
}


static LRESULT
MONTHCAL_HitTest(HWND hwnd, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  PMCHITTESTINFO lpht = (PMCHITTESTINFO)lParam;
  UINT x,y;
  DWORD retval;
  int day,wday,wnum;


  x = lpht->pt.x;
  y = lpht->pt.y;
  retval = MCHT_NOWHERE;


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


static void MONTHCAL_GoToNextMonth(HWND hwnd, MONTHCAL_INFO *infoPtr)
{
  DWORD dwStyle = GetWindowLongA(hwnd, GWL_STYLE);

  TRACE("MONTHCAL_GoToNextMonth\n");

  infoPtr->currentMonth++;
  if(infoPtr->currentMonth > 12) {
    infoPtr->currentYear++;
    infoPtr->currentMonth = 1;
  }

  if(dwStyle & MCS_DAYSTATE) {
    NMDAYSTATE nmds;
    int i;

    nmds.nmhdr.hwndFrom = hwnd;
    nmds.nmhdr.idFrom   = GetWindowLongA(hwnd, GWL_ID);
    nmds.nmhdr.code     = MCN_GETDAYSTATE;
    nmds.cDayState	= infoPtr->monthRange;
    nmds.prgDayState	= Alloc(infoPtr->monthRange * sizeof(MONTHDAYSTATE));

    SendMessageA(infoPtr->hwndNotify, WM_NOTIFY,
    (WPARAM)nmds.nmhdr.idFrom, (LPARAM)&nmds);
    for(i=0; i<infoPtr->monthRange; i++)
      infoPtr->monthdayState[i] = nmds.prgDayState[i];
  }
}


static void MONTHCAL_GoToPrevMonth(HWND hwnd,  MONTHCAL_INFO *infoPtr)
{
  DWORD dwStyle = GetWindowLongA(hwnd, GWL_STYLE);

  TRACE("MONTHCAL_GoToPrevMonth\n");

  infoPtr->currentMonth--;
  if(infoPtr->currentMonth < 1) {
    infoPtr->currentYear--;
    infoPtr->currentMonth = 12;
  }

  if(dwStyle & MCS_DAYSTATE) {
    NMDAYSTATE nmds;
    int i;

    nmds.nmhdr.hwndFrom = hwnd;
    nmds.nmhdr.idFrom   = GetWindowLongA(hwnd, GWL_ID);
    nmds.nmhdr.code     = MCN_GETDAYSTATE;
    nmds.cDayState	= infoPtr->monthRange;
    nmds.prgDayState	= Alloc
                        (infoPtr->monthRange * sizeof(MONTHDAYSTATE));

    SendMessageA(infoPtr->hwndNotify, WM_NOTIFY,
        (WPARAM)nmds.nmhdr.idFrom, (LPARAM)&nmds);
    for(i=0; i<infoPtr->monthRange; i++)
       infoPtr->monthdayState[i] = nmds.prgDayState[i];
  }
}

static LRESULT
MONTHCAL_RButtonDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  HMENU hMenu;
  POINT menupoint;
  char buf[32];

  hMenu = CreatePopupMenu();
  if (!LoadStringA(COMCTL32_hModule,IDM_GOTODAY,buf,sizeof(buf)))
    {
      WARN("Can't load resource\n");
      strcpy(buf,"Go to Today:");
    }
  AppendMenuA(hMenu, MF_STRING|MF_ENABLED,1, buf);
  menupoint.x=(INT)LOWORD(lParam);
  menupoint.y=(INT)HIWORD(lParam);
  ClientToScreen(hwnd, &menupoint);
  if( TrackPopupMenu(hMenu,TPM_RIGHTBUTTON| TPM_NONOTIFY|TPM_RETURNCMD,
		     menupoint.x,menupoint.y,0,hwnd,NULL))
    {
      infoPtr->currentMonth=infoPtr->todaysDate.wMonth;
      infoPtr->currentYear=infoPtr->todaysDate.wYear;
      InvalidateRect(hwnd, NULL, FALSE);
    }
  return 0;
}

static LRESULT
MONTHCAL_LButtonDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  MCHITTESTINFO ht;
  DWORD hit;
  HMENU hMenu;
  RECT rcDay; /* used in determining area to invalidate */
  char buf[32];
  int i;
  POINT menupoint;
  TRACE("%x %lx\n", wParam, lParam);

  if (infoPtr->hWndYearUpDown)
    {
      infoPtr->currentYear=SendMessageA( infoPtr->hWndYearUpDown, UDM_SETPOS,   (WPARAM) 0,(LPARAM)0);
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
      InvalidateRect(hwnd, NULL, FALSE);
    }

  ht.pt.x = (INT)LOWORD(lParam);
  ht.pt.y = (INT)HIWORD(lParam);
  hit = MONTHCAL_HitTest(hwnd, (LPARAM)&ht);

  /* FIXME: these flags should be checked by */
  /*((hit & MCHT_XXX) == MCHT_XXX) b/c some of the flags are */
  /* multi-bit */
  if(hit ==MCHT_TITLEBTNNEXT) {
    MONTHCAL_GoToNextMonth(hwnd, infoPtr);
    infoPtr->status = MC_NEXTPRESSED;
    SetTimer(hwnd, MC_NEXTMONTHTIMER, MC_NEXTMONTHDELAY, 0);
    InvalidateRect(hwnd, NULL, FALSE);
    return TRUE;
  }
  if(hit == MCHT_TITLEBTNPREV){
    MONTHCAL_GoToPrevMonth(hwnd, infoPtr);
    infoPtr->status = MC_PREVPRESSED;
    SetTimer(hwnd, MC_PREVMONTHTIMER, MC_NEXTMONTHDELAY, 0);
    InvalidateRect(hwnd, NULL, FALSE);
    return TRUE;
  }

  if(hit == MCHT_TITLEMONTH) {
    hMenu = CreatePopupMenu();

    for (i=0; i<12;i++)
      {
	GetLocaleInfoA( LOCALE_USER_DEFAULT,LOCALE_SMONTHNAME1+i,
		  buf,sizeof(buf));
	AppendMenuA(hMenu, MF_STRING|MF_ENABLED,i+1, buf);
      }
    menupoint.x=infoPtr->titlemonth.right;
    menupoint.y=infoPtr->titlemonth.bottom;
    ClientToScreen(hwnd, &menupoint);
    i= TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_RETURNCMD,
		      menupoint.x,menupoint.y,0,hwnd,NULL);
    if ((i>0) && (i<13))
      {
	infoPtr->currentMonth=i;
	InvalidateRect(hwnd, NULL, FALSE);
      }
  }
  if(hit == MCHT_TITLEYEAR) {
    infoPtr->hWndYearEdit=CreateWindowExA(0,
			 "EDIT",
			   0,
			 WS_VISIBLE | WS_CHILD |UDS_SETBUDDYINT,
			 infoPtr->titleyear.left+3,infoPtr->titlebtnnext.top,
			 infoPtr->titleyear.right-infoPtr->titleyear.left,
			 infoPtr->textHeight,
			 hwnd,
			 NULL,
			 NULL,
			 NULL);
    infoPtr->hWndYearUpDown=CreateWindowExA(0,
			 UPDOWN_CLASSA,
			   0,
			 WS_VISIBLE | WS_CHILD |UDS_SETBUDDYINT|UDS_NOTHOUSANDS|UDS_ARROWKEYS,
			 infoPtr->titleyear.right+6,infoPtr->titlebtnnext.top,
			 20,
			 infoPtr->textHeight,
			 hwnd,
			 NULL,
			 NULL,
			 NULL);
    SendMessageA( infoPtr->hWndYearUpDown, UDM_SETRANGE, (WPARAM) 0, MAKELONG (9999, 1753));
    SendMessageA( infoPtr->hWndYearUpDown, UDM_SETBUDDY, (WPARAM) infoPtr->hWndYearEdit, (LPARAM)0 );
    SendMessageA( infoPtr->hWndYearUpDown, UDM_SETPOS,   (WPARAM) 0,(LPARAM)infoPtr->currentYear );
    return TRUE;

  }
  if(hit == MCHT_TODAYLINK) {
    infoPtr->currentMonth=infoPtr->todaysDate.wMonth;
    infoPtr->currentYear=infoPtr->todaysDate.wYear;
    InvalidateRect(hwnd, NULL, FALSE);
    return TRUE;
  }
  if(hit == MCHT_CALENDARDATE) {
    SYSTEMTIME selArray[2];
    NMSELCHANGE nmsc;

    MONTHCAL_CopyTime(&ht.st, &selArray[0]);
    MONTHCAL_CopyTime(&ht.st, &selArray[1]);
    MONTHCAL_SetSelRange(hwnd,0,(LPARAM) &selArray);
    MONTHCAL_SetCurSel(hwnd,0,(LPARAM) &selArray);
    TRACE("MCHT_CALENDARDATE\n");
    nmsc.nmhdr.hwndFrom = hwnd;
    nmsc.nmhdr.idFrom   = GetWindowLongA(hwnd, GWL_ID);
    nmsc.nmhdr.code     = MCN_SELCHANGE;
    MONTHCAL_CopyTime(&infoPtr->minSel,&nmsc.stSelStart);
    MONTHCAL_CopyTime(&infoPtr->maxSel,&nmsc.stSelEnd);

    SendMessageA(infoPtr->hwndNotify, WM_NOTIFY,
           (WPARAM)nmsc.nmhdr.idFrom,(LPARAM)&nmsc);


    /* redraw both old and new days if the selected day changed */
    if(infoPtr->curSelDay != ht.st.wDay) {
      MONTHCAL_CalcPosFromDay(infoPtr, ht.st.wDay, ht.st.wMonth, &rcDay);
      InvalidateRect(hwnd, &rcDay, TRUE);

      MONTHCAL_CalcPosFromDay(infoPtr, infoPtr->curSelDay, infoPtr->currentMonth, &rcDay);
      InvalidateRect(hwnd, &rcDay, TRUE);
    }

    infoPtr->firstSelDay = ht.st.wDay;
    infoPtr->curSelDay = ht.st.wDay;
    infoPtr->status = MC_SEL_LBUTDOWN;
    return TRUE;
  }

  return 0;
}


static LRESULT
MONTHCAL_LButtonUp(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  NMSELCHANGE nmsc;
  NMHDR nmhdr;
  BOOL redraw = FALSE;
  MCHITTESTINFO ht;
  DWORD hit;

  TRACE("\n");

  if(infoPtr->status & MC_NEXTPRESSED) {
    KillTimer(hwnd, MC_NEXTMONTHTIMER);
    infoPtr->status &= ~MC_NEXTPRESSED;
    redraw = TRUE;
  }
  if(infoPtr->status & MC_PREVPRESSED) {
    KillTimer(hwnd, MC_PREVMONTHTIMER);
    infoPtr->status &= ~MC_PREVPRESSED;
    redraw = TRUE;
  }

  ht.pt.x = (INT)LOWORD(lParam);
  ht.pt.y = (INT)HIWORD(lParam);
  hit = MONTHCAL_HitTest(hwnd, (LPARAM)&ht);

  infoPtr->status = MC_SEL_LBUTUP;

  if(hit ==MCHT_CALENDARDATENEXT) {
    MONTHCAL_GoToNextMonth(hwnd, infoPtr);
    InvalidateRect(hwnd, NULL, FALSE);
    return TRUE;
  }
  if(hit == MCHT_CALENDARDATEPREV){
    MONTHCAL_GoToPrevMonth(hwnd, infoPtr);
    InvalidateRect(hwnd, NULL, FALSE);
    return TRUE;
  }
  nmhdr.hwndFrom = hwnd;
  nmhdr.idFrom   = GetWindowLongA( hwnd, GWL_ID);
  nmhdr.code     = NM_RELEASEDCAPTURE;
  TRACE("Sent notification from %p to %p\n", hwnd, infoPtr->hwndNotify);

  SendMessageA(infoPtr->hwndNotify, WM_NOTIFY,
                                (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);
  /* redraw if necessary */
  if(redraw)
    InvalidateRect(hwnd, NULL, FALSE);
  /* only send MCN_SELECT if currently displayed month's day was selected */
  if(hit == MCHT_CALENDARDATE) {
    nmsc.nmhdr.hwndFrom = hwnd;
    nmsc.nmhdr.idFrom   = GetWindowLongA(hwnd, GWL_ID);
    nmsc.nmhdr.code     = MCN_SELECT;
    MONTHCAL_CopyTime(&infoPtr->minSel, &nmsc.stSelStart);
    MONTHCAL_CopyTime(&infoPtr->maxSel, &nmsc.stSelEnd);

    SendMessageA(infoPtr->hwndNotify, WM_NOTIFY,
             (WPARAM)nmsc.nmhdr.idFrom, (LPARAM)&nmsc);

  }
  return 0;
}


static LRESULT
MONTHCAL_Timer(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  BOOL redraw = FALSE;

  TRACE(" %d\n", wParam);
  if(!infoPtr) return 0;

  switch(wParam) {
  case MC_NEXTMONTHTIMER:
    redraw = TRUE;
    MONTHCAL_GoToNextMonth(hwnd, infoPtr);
    break;
  case MC_PREVMONTHTIMER:
    redraw = TRUE;
    MONTHCAL_GoToPrevMonth(hwnd, infoPtr);
    break;
  default:
    ERR("got unknown timer\n");
  }

  /* redraw only if necessary */
  if(redraw)
    InvalidateRect(hwnd, NULL, FALSE);

  return 0;
}


static LRESULT
MONTHCAL_MouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  MCHITTESTINFO ht;
  int oldselday, selday, hit;
  RECT r;

  if(!(infoPtr->status & MC_SEL_LBUTDOWN)) return 0;

  ht.pt.x = LOWORD(lParam);
  ht.pt.y = HIWORD(lParam);

  hit = MONTHCAL_HitTest(hwnd, (LPARAM)&ht);

  /* not on the calendar date numbers? bail out */
  TRACE("hit:%x\n",hit);
  if((hit & MCHT_CALENDARDATE) != MCHT_CALENDARDATE) return 0;

  selday = ht.st.wDay;
  oldselday = infoPtr->curSelDay;
  infoPtr->curSelDay = selday;
  MONTHCAL_CalcPosFromDay(infoPtr, selday, ht.st. wMonth, &r);

  if(GetWindowLongA(hwnd, GWL_STYLE) & MCS_MULTISELECT)  {
    SYSTEMTIME selArray[2];
    int i;

    MONTHCAL_GetSelRange(hwnd, 0, (LPARAM)&selArray);
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

      MONTHCAL_SetSelRange(hwnd, 0, (LPARAM)&selArray);
    }
  }

done:

  /* only redraw if the currently selected day changed */
  /* FIXME: this should specify a rectangle containing only the days that changed */
  /* using InvalidateRect */
  if(oldselday != infoPtr->curSelDay)
    InvalidateRect(hwnd, NULL, FALSE);

  return 0;
}


static LRESULT
MONTHCAL_Paint(HWND hwnd, WPARAM wParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  HDC hdc;
  PAINTSTRUCT ps;

  /* fill ps.rcPaint with a default rect */
  memcpy(&(ps.rcPaint), &(infoPtr->rcClient), sizeof(infoPtr->rcClient));

  hdc = (wParam==0 ? BeginPaint(hwnd, &ps) : (HDC)wParam);
  MONTHCAL_Refresh(hwnd, hdc, &ps);
  if(!wParam) EndPaint(hwnd, &ps);
  return 0;
}


static LRESULT
MONTHCAL_KillFocus(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TRACE("\n");

  InvalidateRect(hwnd, NULL, TRUE);

  return 0;
}


static LRESULT
MONTHCAL_SetFocus(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TRACE("\n");

  InvalidateRect(hwnd, NULL, FALSE);

  return 0;
}

/* sets the size information */
static void MONTHCAL_UpdateSize(HWND hwnd)
{
  HDC hdc = GetDC(hwnd);
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  RECT *rcClient=&infoPtr->rcClient;
  RECT *rcDraw=&infoPtr->rcDraw;
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
  TEXTMETRICA tm;
  DWORD dwStyle = GetWindowLongA(hwnd, GWL_STYLE);
  HFONT currentFont;
  double xdiv;

  currentFont = SelectObject(hdc, infoPtr->hFont);

  /* FIXME: need a way to determine current font, without setting it */
  /*
  if(infoPtr->hFont!=currentFont) {
    SelectObject(hdc, currentFont);
    infoPtr->hFont=currentFont;
    GetObjectA(currentFont, sizeof(LOGFONTA), &logFont);
    logFont.lfWeight=FW_BOLD;
    infoPtr->hBoldFont = CreateFontIndirectA(&logFont);
  }
  */

  /* get the height and width of each day's text */
  GetTextMetricsA(hdc, &tm);
  infoPtr->textHeight = tm.tmHeight + tm.tmExternalLeading;
  GetTextExtentPoint32A(hdc, "Sun", 3, &size);
  infoPtr->textWidth = size.cx + 2;

  /* retrieve the controls client rectangle info infoPtr->rcClient */
  GetClientRect(hwnd, rcClient);

  /* rcDraw is the rectangle the control is drawn in */
  rcDraw->left = rcClient->left;
  rcDraw->right = rcClient->right;
  rcDraw->top = rcClient->top;
  rcDraw->bottom = rcClient->bottom;

  /* recalculate the height and width increments and offsets */
  /* FIXME: We use up all available width. This will inhibit having multiple
     calendars in a row, like win doesn
  */
  if(dwStyle & MCS_WEEKNUMBERS)
    xdiv=8.0;
  else
    xdiv=7.0;
  infoPtr->width_increment = (infoPtr->rcDraw.right - infoPtr->rcDraw.left) / xdiv;
  infoPtr->height_increment = (infoPtr->rcDraw.bottom - infoPtr->rcDraw.top) / 10.0;
  infoPtr->left_offset = (infoPtr->rcDraw.right - infoPtr->rcDraw.left) - (infoPtr->width_increment * xdiv);
  infoPtr->top_offset = (infoPtr->rcDraw.bottom - infoPtr->rcDraw.top) - (infoPtr->height_increment * 10.0);

  rcDraw->bottom = rcDraw->top + 10 * infoPtr->height_increment;
  /* this is correct, the control does NOT expand vertically */
  /* like it does horizontally */
  /* make sure we don't move the controls bottom out of the client */
  /* area */
  /* title line has about 3 text heights, abrev days line, 6 weeksline and today circle line*/
  /*if((rcDraw->top + 9 * infoPtr->textHeight + 5) < rcDraw->bottom) {
    rcDraw->bottom = rcDraw->top + 9 * infoPtr->textHeight + 5;
    }*/

  /* calculate title area */
  title->top    = rcClient->top;
  title->bottom = title->top + 2 * infoPtr->height_increment;
  title->left   = rcClient->left;
  title->right  = rcClient->right;

  /* set the dimensions of the next and previous buttons and center */
  /* the month text vertically */
  prev->top    = next->top    = title->top + 6;
  prev->bottom = next->bottom = title->bottom - 6;
  prev->left   = title->left  + 6;
  prev->right  = prev->left + (title->bottom - title->top) ;
  next->right  = title->right - 6;
  next->left   = next->right - (title->bottom - title->top);

  /* titlemonth->left and right change based upon the current month */
  /* and are recalculated in refresh as the current month may change */
  /* without the control being resized */
  titlemonth->top    = titleyear->top    = title->top    + (infoPtr->height_increment)/2;
  titlemonth->bottom = titleyear->bottom = title->bottom - (infoPtr->height_increment)/2;

  /* setup the dimensions of the rectangle we draw the names of the */
  /* days of the week in */
  weeknumrect->left =infoPtr->left_offset;
  if(dwStyle & MCS_WEEKNUMBERS)
    weeknumrect->right=prev->right;
  else
    weeknumrect->right=weeknumrect->left;
  wdays->left   = days->left   = weeknumrect->right;
  wdays->right  = days->right  = wdays->left + 7 * infoPtr->width_increment;
  wdays->top    = title->bottom ;
  wdays->bottom = wdays->top + infoPtr->height_increment;

  days->top    = weeknumrect->top = wdays->bottom ;
  days->bottom = weeknumrect->bottom = days->top     + 6 * infoPtr->height_increment;

  todayrect->left   = rcClient->left;
  todayrect->right  = rcClient->right;
  todayrect->top    = days->bottom;
  todayrect->bottom = days->bottom + infoPtr->height_increment;

  /* uncomment for excessive debugging
  TRACE("dx=%d dy=%d rcC[%d %d %d %d] t[%d %d %d %d] wd[%d %d %d %d] w[%d %d %d %d] t[%d %d %d %d]\n",
	infoPtr->width_increment,infoPtr->height_increment,
	 rcClient->left, rcClient->right, rcClient->top, rcClient->bottom,
	    title->left,    title->right,    title->top,    title->bottom,
	    wdays->left,    wdays->right,    wdays->top,    wdays->bottom,
	     days->left,     days->right,     days->top,     days->bottom,
	todayrect->left,todayrect->right,todayrect->top,todayrect->bottom);
  */

  /* restore the originally selected font */
  SelectObject(hdc, currentFont);

  ReleaseDC(hwnd, hdc);
}

static LRESULT MONTHCAL_Size(HWND hwnd, int Width, int Height)
{
  TRACE("(hwnd=%p, width=%d, height=%d)\n", hwnd, Width, Height);

  MONTHCAL_UpdateSize(hwnd);

  /* invalidate client area and erase background */
  InvalidateRect(hwnd, NULL, TRUE);

  return 0;
}

/* FIXME: check whether dateMin/dateMax need to be adjusted. */
static LRESULT
MONTHCAL_Create(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr;
  LOGFONTA	logFont;

  /* allocate memory for info structure */
  infoPtr =(MONTHCAL_INFO*)Alloc(sizeof(MONTHCAL_INFO));
  SetWindowLongA(hwnd, 0, (DWORD)infoPtr);

  if(infoPtr == NULL) {
    ERR( "could not allocate info memory!\n");
    return 0;
  }
  if((MONTHCAL_INFO*)GetWindowLongA(hwnd, 0) != infoPtr) {
    ERR( "pointer assignment error!\n");
    return 0;
  }

  infoPtr->hwndNotify = ((LPCREATESTRUCTW)lParam)->hwndParent;

  infoPtr->hFont = GetStockObject(DEFAULT_GUI_FONT);
  GetObjectA(infoPtr->hFont, sizeof(LOGFONTA), &logFont);
  logFont.lfWeight = FW_BOLD;
  infoPtr->hBoldFont = CreateFontIndirectA(&logFont);

  /* initialize info structure */
  /* FIXME: calculate systemtime ->> localtime(substract timezoneinfo) */

  GetSystemTime(&infoPtr->todaysDate);
  MONTHCAL_SetFirstDayOfWeek(hwnd,0,(LPARAM)-1);
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

  /* call MONTHCAL_UpdateSize to set all of the dimensions */
  /* of the control */
  MONTHCAL_UpdateSize(hwnd);

  return 0;
}


static LRESULT
MONTHCAL_Destroy(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  /* free month calendar info data */
  if(infoPtr->monthdayState)
      Free(infoPtr->monthdayState);
  Free(infoPtr);
  SetWindowLongA(hwnd, 0, 0);
  return 0;
}


static LRESULT WINAPI
MONTHCAL_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  TRACE("hwnd=%p msg=%x wparam=%x lparam=%lx\n", hwnd, uMsg, wParam, lParam);
  if (!MONTHCAL_GetInfoPtr(hwnd) && (uMsg != WM_CREATE))
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
  switch(uMsg)
  {
  case MCM_GETCURSEL:
    return MONTHCAL_GetCurSel(hwnd, wParam, lParam);

  case MCM_SETCURSEL:
    return MONTHCAL_SetCurSel(hwnd, wParam, lParam);

  case MCM_GETMAXSELCOUNT:
    return MONTHCAL_GetMaxSelCount(hwnd, wParam, lParam);

  case MCM_SETMAXSELCOUNT:
    return MONTHCAL_SetMaxSelCount(hwnd, wParam, lParam);

  case MCM_GETSELRANGE:
    return MONTHCAL_GetSelRange(hwnd, wParam, lParam);

  case MCM_SETSELRANGE:
    return MONTHCAL_SetSelRange(hwnd, wParam, lParam);

  case MCM_GETMONTHRANGE:
    return MONTHCAL_GetMonthRange(hwnd, wParam, lParam);

  case MCM_SETDAYSTATE:
    return MONTHCAL_SetDayState(hwnd, wParam, lParam);

  case MCM_GETMINREQRECT:
    return MONTHCAL_GetMinReqRect(hwnd, wParam, lParam);

  case MCM_GETCOLOR:
    return MONTHCAL_GetColor(hwnd, wParam, lParam);

  case MCM_SETCOLOR:
    return MONTHCAL_SetColor(hwnd, wParam, lParam);

  case MCM_GETTODAY:
    return MONTHCAL_GetToday(hwnd, wParam, lParam);

  case MCM_SETTODAY:
    return MONTHCAL_SetToday(hwnd, wParam, lParam);

  case MCM_HITTEST:
    return MONTHCAL_HitTest(hwnd,lParam);

  case MCM_GETFIRSTDAYOFWEEK:
    return MONTHCAL_GetFirstDayOfWeek(hwnd, wParam, lParam);

  case MCM_SETFIRSTDAYOFWEEK:
    return MONTHCAL_SetFirstDayOfWeek(hwnd, wParam, lParam);

  case MCM_GETRANGE:
    return MONTHCAL_GetRange(hwnd, wParam, lParam);

  case MCM_SETRANGE:
    return MONTHCAL_SetRange(hwnd, wParam, lParam);

  case MCM_GETMONTHDELTA:
    return MONTHCAL_GetMonthDelta(hwnd, wParam, lParam);

  case MCM_SETMONTHDELTA:
    return MONTHCAL_SetMonthDelta(hwnd, wParam, lParam);

  case MCM_GETMAXTODAYWIDTH:
    return MONTHCAL_GetMaxTodayWidth(hwnd);

  case WM_GETDLGCODE:
    return DLGC_WANTARROWS | DLGC_WANTCHARS;

  case WM_KILLFOCUS:
    return MONTHCAL_KillFocus(hwnd, wParam, lParam);

  case WM_RBUTTONDOWN:
    return MONTHCAL_RButtonDown(hwnd, wParam, lParam);

  case WM_LBUTTONDOWN:
    return MONTHCAL_LButtonDown(hwnd, wParam, lParam);

  case WM_MOUSEMOVE:
    return MONTHCAL_MouseMove(hwnd, wParam, lParam);

  case WM_LBUTTONUP:
    return MONTHCAL_LButtonUp(hwnd, wParam, lParam);

  case WM_PAINT:
    return MONTHCAL_Paint(hwnd, wParam);

  case WM_SETFOCUS:
    return MONTHCAL_SetFocus(hwnd, wParam, lParam);

  case WM_SIZE:
    return MONTHCAL_Size(hwnd, (short)LOWORD(lParam), (short)HIWORD(lParam));

  case WM_CREATE:
    return MONTHCAL_Create(hwnd, wParam, lParam);

  case WM_TIMER:
    return MONTHCAL_Timer(hwnd, wParam, lParam);

  case WM_DESTROY:
    return MONTHCAL_Destroy(hwnd, wParam, lParam);

  default:
    if ((uMsg >= WM_USER) && (uMsg < WM_APP))
      ERR( "unknown msg %04x wp=%08x lp=%08lx\n", uMsg, wParam, lParam);
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
  }
  return 0;
}


void
MONTHCAL_Register(void)
{
  WNDCLASSA wndClass;

  ZeroMemory(&wndClass, sizeof(WNDCLASSA));
  wndClass.style         = CS_GLOBALCLASS;
  wndClass.lpfnWndProc   = (WNDPROC)MONTHCAL_WindowProc;
  wndClass.cbClsExtra    = 0;
  wndClass.cbWndExtra    = sizeof(MONTHCAL_INFO *);
  wndClass.hCursor       = LoadCursorA(0, (LPSTR)IDC_ARROW);
  wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wndClass.lpszClassName = MONTHCAL_CLASSA;

  RegisterClassA(&wndClass);
}


void
MONTHCAL_Unregister(void)
{
    UnregisterClassA(MONTHCAL_CLASSA, NULL);
}
