/*
 * Date and time picker control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 1999, 2000 Alex Priem <alexp@sci.kun.nl>
 * Copyright 2000 Chris Morgan <cmorgan@wpi.edu>
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
 *    -- DTS_APPCANPARSE
 *    -- DTS_SHORTDATECENTURYFORMAT
 *    -- DTN_CLOSEUP
 *    -- DTN_FORMAT
 *    -- DTN_FORMATQUERY
 *    -- DTN_USERSTRING
 *    -- DTN_WMKEYDOWN
 *    -- FORMATCALLBACK
 */

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(datetime);

typedef struct
{
    HWND hwndSelf;
    HWND hMonthCal;
    HWND hwndNotify;
    HWND hUpdown;
    DWORD dwStyle;
    SYSTEMTIME date;
    BOOL dateValid;
    HWND hwndCheckbut;
    RECT rcClient; /* rect around the edge of the window */
    RECT rcDraw; /* rect inside of the border */
    RECT checkbox;  /* checkbox allowing the control to be enabled/disabled */
    RECT calbutton; /* button that toggles the dropdown of the monthcal control */
    BOOL bCalDepressed; /* TRUE = cal button is depressed */
    int  select;
    HFONT hFont;
    int nrFieldsAllocated;
    int nrFields;
    int haveFocus;
    int *fieldspec;
    RECT *fieldRect;
    int  *buflen;
    WCHAR textbuf[256];
    POINT monthcal_pos;
    int pendingUpdown;
} DATETIME_INFO, *LPDATETIME_INFO;

/* in monthcal.c */
extern int MONTHCAL_MonthLength(int month, int year);

/* this list of defines is closely related to `allowedformatchars' defined
 * in datetime.c; the high nibble indicates the `base type' of the format
 * specifier.
 * Do not change without first reading DATETIME_UseFormat.
 *
 */

#define DT_END_FORMAT      0
#define ONEDIGITDAY   	0x01
#define TWODIGITDAY   	0x02
#define THREECHARDAY  	0x03
#define FULLDAY         0x04
#define ONEDIGIT12HOUR  0x11
#define TWODIGIT12HOUR  0x12
#define ONEDIGIT24HOUR  0x21
#define TWODIGIT24HOUR  0x22
#define ONEDIGITMINUTE  0x31
#define TWODIGITMINUTE  0x32
#define ONEDIGITMONTH   0x41
#define TWODIGITMONTH   0x42
#define THREECHARMONTH  0x43
#define FULLMONTH       0x44
#define ONEDIGITSECOND  0x51
#define TWODIGITSECOND  0x52
#define ONELETTERAMPM   0x61
#define TWOLETTERAMPM   0x62
#define ONEDIGITYEAR    0x71
#define TWODIGITYEAR    0x72
#define INVALIDFULLYEAR 0x73      /* FIXME - yyy is not valid - we'll treat it as yyyy */
#define FULLYEAR        0x74
#define FORMATCALLBACK  0x81      /* -> maximum of 0x80 callbacks possible */
#define FORMATCALLMASK  0x80
#define DT_STRING 	0x0100

#define DTHT_DATEFIELD  0xff      /* for hit-testing */

#define DTHT_NONE     0
#define DTHT_CHECKBOX 0x200	/* these should end at '00' , to make */
#define DTHT_MCPOPUP  0x300     /* & DTHT_DATEFIELD 0 when DATETIME_KeyDown */
#define DTHT_GOTFOCUS 0x400     /* tests for date-fields */

static BOOL DATETIME_SendSimpleNotify (const DATETIME_INFO *infoPtr, UINT code);
static BOOL DATETIME_SendDateTimeChangeNotify (const DATETIME_INFO *infoPtr);
extern void MONTHCAL_CopyTime(const SYSTEMTIME *from, SYSTEMTIME *to);
static const WCHAR allowedformatchars[] = {'d', 'h', 'H', 'm', 'M', 's', 't', 'y', 'X', '\'', 0};
static const int maxrepetition [] = {4,2,2,2,4,2,2,4,-1,-1};


static DWORD
DATETIME_GetSystemTime (const DATETIME_INFO *infoPtr, SYSTEMTIME *lprgSysTimeArray)
{
    if (!lprgSysTimeArray) return GDT_NONE;

    if ((infoPtr->dwStyle & DTS_SHOWNONE) &&
        (SendMessageW (infoPtr->hwndCheckbut, BM_GETCHECK, 0, 0) == BST_UNCHECKED))
        return GDT_NONE;

    MONTHCAL_CopyTime (&infoPtr->date, lprgSysTimeArray);

    return GDT_VALID;
}


static BOOL
DATETIME_SetSystemTime (DATETIME_INFO *infoPtr, DWORD flag, const SYSTEMTIME *lprgSysTimeArray)
{
    if (!lprgSysTimeArray) return 0;

    TRACE("%04d/%02d/%02d %02d:%02d:%02d\n",
          lprgSysTimeArray->wYear, lprgSysTimeArray->wMonth, lprgSysTimeArray->wDay,
          lprgSysTimeArray->wHour, lprgSysTimeArray->wMinute, lprgSysTimeArray->wSecond);

    if (flag == GDT_VALID) {
      if (lprgSysTimeArray->wYear < 1601 || lprgSysTimeArray->wYear > 30827 ||
          lprgSysTimeArray->wMonth < 1 || lprgSysTimeArray->wMonth > 12 ||
          lprgSysTimeArray->wDayOfWeek > 6 ||
          lprgSysTimeArray->wDay < 1 || lprgSysTimeArray->wDay > 31 ||
          lprgSysTimeArray->wHour > 23 ||
          lprgSysTimeArray->wMinute > 59 ||
          lprgSysTimeArray->wSecond > 59 ||
          lprgSysTimeArray->wMilliseconds > 999
          )
        return 0;

        infoPtr->dateValid = TRUE;
        MONTHCAL_CopyTime (lprgSysTimeArray, &infoPtr->date);
        SendMessageW (infoPtr->hMonthCal, MCM_SETCURSEL, 0, (LPARAM)(&infoPtr->date));
        SendMessageW (infoPtr->hwndCheckbut, BM_SETCHECK, BST_CHECKED, 0);
    } else if ((infoPtr->dwStyle & DTS_SHOWNONE) && (flag == GDT_NONE)) {
        infoPtr->dateValid = FALSE;
        SendMessageW (infoPtr->hwndCheckbut, BM_SETCHECK, BST_UNCHECKED, 0);
    }
    else
        return 0;

    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return TRUE;
}


/***
 * Split up a formattxt in actions.
 * See ms documentation for the meaning of the letter codes/'specifiers'.
 *
 * Notes:
 * *'dddddd' is handled as 'dddd' plus 'dd'.
 * *unrecognized formats are strings (here given the type DT_STRING;
 * start of the string is encoded in lower bits of DT_STRING.
 * Therefore, 'string' ends finally up as '<show seconds>tring'.
 *
 */
static void
DATETIME_UseFormat (DATETIME_INFO *infoPtr, LPCWSTR formattxt)
{
    unsigned int i;
    int j, k, len;
    int *nrFields = &infoPtr->nrFields;

    *nrFields = 0;
    infoPtr->fieldspec[*nrFields] = 0;
    len = strlenW(allowedformatchars);
    k = 0;

    for (i = 0; formattxt[i]; i++)  {
	TRACE ("\n%d %c:", i, formattxt[i]);
 	for (j = 0; j < len; j++) {
 	    if (allowedformatchars[j]==formattxt[i]) {
		TRACE ("%c[%d,%x]", allowedformatchars[j], *nrFields, infoPtr->fieldspec[*nrFields]);
		if ((*nrFields==0) && (infoPtr->fieldspec[*nrFields]==0)) {
		    infoPtr->fieldspec[*nrFields] = (j<<4) + 1;
		    break;
		}
		if (infoPtr->fieldspec[*nrFields] >> 4 != j) {
		    (*nrFields)++;
		    infoPtr->fieldspec[*nrFields] = (j<<4) + 1;
		    break;
		}
		if ((infoPtr->fieldspec[*nrFields] & 0x0f) == maxrepetition[j]) {
		    (*nrFields)++;
		    infoPtr->fieldspec[*nrFields] = (j<<4) + 1;
		    break;
		}
		infoPtr->fieldspec[*nrFields]++;
		break;
	    }   /* if allowedformatchar */
	} /* for j */

	/* char is not a specifier: handle char like a string */
	if (j == len) {
	    if ((*nrFields==0) && (infoPtr->fieldspec[*nrFields]==0)) {
		infoPtr->fieldspec[*nrFields] = DT_STRING + k;
		infoPtr->buflen[*nrFields] = 0;
            } else if ((infoPtr->fieldspec[*nrFields] & DT_STRING) != DT_STRING)  {
		(*nrFields)++;
		infoPtr->fieldspec[*nrFields] = DT_STRING + k;
		infoPtr->buflen[*nrFields] = 0;
	    }
	    infoPtr->textbuf[k] = formattxt[i];
	    k++;
	    infoPtr->buflen[*nrFields]++;
	}   /* if j=len */

	if (*nrFields == infoPtr->nrFieldsAllocated) {
	    FIXME ("out of memory; should reallocate. crash ahead.\n");
	}
    } /* for i */

    TRACE("\n");

    if (infoPtr->fieldspec[*nrFields] != 0) (*nrFields)++;
}


static BOOL
DATETIME_SetFormatW (DATETIME_INFO *infoPtr, LPCWSTR lpszFormat)
{
    if (!lpszFormat) {
	WCHAR format_buf[80];
	DWORD format_item;

	if (infoPtr->dwStyle & DTS_LONGDATEFORMAT)
	    format_item = LOCALE_SLONGDATE;
	else if ((infoPtr->dwStyle & DTS_TIMEFORMAT) == DTS_TIMEFORMAT)
	    format_item = LOCALE_STIMEFORMAT;
        else /* DTS_SHORTDATEFORMAT */
	    format_item = LOCALE_SSHORTDATE;
	GetLocaleInfoW( GetSystemDefaultLCID(), format_item, format_buf, sizeof(format_buf)/sizeof(format_buf[0]));
	lpszFormat = format_buf;
    }

    DATETIME_UseFormat (infoPtr, lpszFormat);
    InvalidateRect (infoPtr->hwndSelf, NULL, TRUE);

    return 1;
}


static BOOL
DATETIME_SetFormatA (DATETIME_INFO *infoPtr, LPCSTR lpszFormat)
{
    if (lpszFormat) {
	BOOL retval;
	INT len = MultiByteToWideChar(CP_ACP, 0, lpszFormat, -1, NULL, 0);
	LPWSTR wstr = Alloc(len * sizeof(WCHAR));
	if (wstr) MultiByteToWideChar(CP_ACP, 0, lpszFormat, -1, wstr, len);
	retval = DATETIME_SetFormatW (infoPtr, wstr);
	Free (wstr);
	return retval;
    }
    else
	return DATETIME_SetFormatW (infoPtr, 0);

}


static void
DATETIME_ReturnTxt (const DATETIME_INFO *infoPtr, int count, LPWSTR result, int resultSize)
{
    static const WCHAR fmt_dW[] = { '%', 'd', 0 };
    static const WCHAR fmt__2dW[] = { '%', '.', '2', 'd', 0 };
    static const WCHAR fmt__3sW[] = { '%', '.', '3', 's', 0 };
    SYSTEMTIME date = infoPtr->date;
    int spec;
    WCHAR buffer[80];

    *result=0;
    TRACE ("%d,%d\n", infoPtr->nrFields, count);
    if (count>infoPtr->nrFields || count < 0) {
	WARN ("buffer overrun, have %d want %d\n", infoPtr->nrFields, count);
	return;
    }

    if (!infoPtr->fieldspec) return;

    spec = infoPtr->fieldspec[count];
    if (spec & DT_STRING) {
	int txtlen = infoPtr->buflen[count];

        if (txtlen > resultSize)
            txtlen = resultSize - 1;
	memcpy (result, infoPtr->textbuf + (spec &~ DT_STRING), txtlen * sizeof(WCHAR));
	result[txtlen] = 0;
	TRACE ("arg%d=%x->[%s]\n", count, infoPtr->fieldspec[count], debugstr_w(result));
	return;
    }


    switch (spec) {
	case DT_END_FORMAT:
	    *result = 0;
	    break;
	case ONEDIGITDAY:
	    wsprintfW (result, fmt_dW, date.wDay);
	    break;
	case TWODIGITDAY:
	    wsprintfW (result, fmt__2dW, date.wDay);
	    break;
	case THREECHARDAY:
	    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SABBREVDAYNAME1+(date.wDayOfWeek+6)%7, result, 4);
	    /*wsprintfW (result,"%.3s",days[date.wDayOfWeek]);*/
	    break;
	case FULLDAY:
	    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDAYNAME1+(date.wDayOfWeek+6)%7, result, resultSize);
	    break;
	case ONEDIGIT12HOUR:
	    if (date.wHour == 0) {
	        result[0] = '1';
	        result[1] = '2';
	        result[2] = 0;
	    }
	    else
	        wsprintfW (result, fmt_dW, date.wHour - (date.wHour > 12 ? 12 : 0));
	    break;
	case TWODIGIT12HOUR:
	    if (date.wHour == 0) {
	        result[0] = '1';
	        result[1] = '2';
	        result[2] = 0;
	    }
	    else
	        wsprintfW (result, fmt__2dW, date.wHour - (date.wHour > 12 ? 12 : 0));
	    break;
	case ONEDIGIT24HOUR:
	    wsprintfW (result, fmt_dW, date.wHour);
	    break;
	case TWODIGIT24HOUR:
	    wsprintfW (result, fmt__2dW, date.wHour);
	    break;
	case ONEDIGITSECOND:
	    wsprintfW (result, fmt_dW, date.wSecond);
	    break;
	case TWODIGITSECOND:
	    wsprintfW (result, fmt__2dW, date.wSecond);
	    break;
	case ONEDIGITMINUTE:
	    wsprintfW (result, fmt_dW, date.wMinute);
	    break;
	case TWODIGITMINUTE:
	    wsprintfW (result, fmt__2dW, date.wMinute);
	    break;
	case ONEDIGITMONTH:
	    wsprintfW (result, fmt_dW, date.wMonth);
	    break;
	case TWODIGITMONTH:
	    wsprintfW (result, fmt__2dW, date.wMonth);
	    break;
	case THREECHARMONTH:
	    GetLocaleInfoW(GetSystemDefaultLCID(), LOCALE_SMONTHNAME1+date.wMonth -1, 
			   buffer, sizeof(buffer)/sizeof(buffer[0]));
	    wsprintfW (result, fmt__3sW, buffer);
	    break;
	case FULLMONTH:
	    GetLocaleInfoW(GetSystemDefaultLCID(),LOCALE_SMONTHNAME1+date.wMonth -1,
                           result, resultSize);
	    break;
	case ONELETTERAMPM:
	    result[0] = (date.wHour < 12 ? 'A' : 'P');
	    result[1] = 0;
	    break;
	case TWOLETTERAMPM:
	    result[0] = (date.wHour < 12 ? 'A' : 'P');
	    result[1] = 'M';
	    result[2] = 0;
	    break;
	case FORMATCALLBACK:
	    FIXME ("Not implemented\n");
	    result[0] = 'x';
	    result[1] = 0;
	    break;
	case ONEDIGITYEAR:
	    wsprintfW (result, fmt_dW, date.wYear-10* (int) floor(date.wYear/10));
	    break;
	case TWODIGITYEAR:
	    wsprintfW (result, fmt__2dW, date.wYear-100* (int) floor(date.wYear/100));
	    break;
        case INVALIDFULLYEAR:
	case FULLYEAR:
	    wsprintfW (result, fmt_dW, date.wYear);
	    break;
    }

    TRACE ("arg%d=%x->[%s]\n", count, infoPtr->fieldspec[count], debugstr_w(result));
}

/* Offsets of days in the week to the weekday of january 1 in a leap year. */
static const int DayOfWeekTable[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

/* returns the day in the week(0 == sunday, 6 == saturday) */
/* day(1 == 1st, 2 == 2nd... etc), year is the  year value */
static int DATETIME_CalculateDayOfWeek(DWORD day, DWORD month, DWORD year)
{
    year-=(month < 3);
    
    return((year + year/4 - year/100 + year/400 +
         DayOfWeekTable[month-1] + day ) % 7);
}

static int wrap(int val, int delta, int minVal, int maxVal)
{
    val += delta;
    if (delta == INT_MIN || val < minVal) return maxVal;
    if (delta == INT_MAX || val > maxVal) return minVal;
    return val;
}

static void
DATETIME_IncreaseField (DATETIME_INFO *infoPtr, int number, int delta)
{
    SYSTEMTIME *date = &infoPtr->date;

    TRACE ("%d\n", number);
    if ((number > infoPtr->nrFields) || (number < 0)) return;

    if ((infoPtr->fieldspec[number] & DTHT_DATEFIELD) == 0) return;

    switch (infoPtr->fieldspec[number]) {
	case ONEDIGITYEAR:
	case TWODIGITYEAR:
	case FULLYEAR:
	    date->wYear = wrap(date->wYear, delta, 1752, 9999);
	    date->wDayOfWeek = DATETIME_CalculateDayOfWeek(date->wDay,date->wMonth,date->wYear);
	    break;
	case ONEDIGITMONTH:
	case TWODIGITMONTH:
	case THREECHARMONTH:
	case FULLMONTH:
	    date->wMonth = wrap(date->wMonth, delta, 1, 12);
	    date->wDayOfWeek = DATETIME_CalculateDayOfWeek(date->wDay,date->wMonth,date->wYear);
	    delta = 0;
	    /* fall through */
	case ONEDIGITDAY:
	case TWODIGITDAY:
	case THREECHARDAY:
	case FULLDAY:
	    date->wDay = wrap(date->wDay, delta, 1, MONTHCAL_MonthLength(date->wMonth, date->wYear));
	    date->wDayOfWeek = DATETIME_CalculateDayOfWeek(date->wDay,date->wMonth,date->wYear);
	    break;
	case ONELETTERAMPM:
	case TWOLETTERAMPM:
	    delta *= 12;
	    /* fall through */
	case ONEDIGIT12HOUR:
	case TWODIGIT12HOUR:
	case ONEDIGIT24HOUR:
	case TWODIGIT24HOUR:
	    date->wHour = wrap(date->wHour, delta, 0, 23);
	    break;
	case ONEDIGITMINUTE:
	case TWODIGITMINUTE:
	    date->wMinute = wrap(date->wMinute, delta, 0, 59);
	    break;
	case ONEDIGITSECOND:
	case TWODIGITSECOND:
	    date->wSecond = wrap(date->wSecond, delta, 0, 59);
	    break;
	case FORMATCALLBACK:
	    FIXME ("Not implemented\n");
	    break;
    }

    /* FYI: On 1752/9/14 the calendar changed and England and the
     * American colonies changed to the Gregorian calendar. This change
     * involved having September 14th follow September 2nd. So no date
     * algorithm works before that date.
     */
    if (10000 * date->wYear + 100 * date->wMonth + date->wDay < 17520914) {
	date->wYear = 1752;
    	date->wMonth = 9;
	date->wDay = 14;
	date->wSecond = 0;
	date->wMinute = 0;
	date->wHour = 0;
    }
}


static void
DATETIME_ReturnFieldWidth (const DATETIME_INFO *infoPtr, HDC hdc, int count, SHORT *fieldWidthPtr)
{
    /* fields are a fixed width, determined by the largest possible string */
    /* presumably, these widths should be language dependent */
    static const WCHAR fld_d1W[] = { '2', 0 };
    static const WCHAR fld_d2W[] = { '2', '2', 0 };
    static const WCHAR fld_d4W[] = { '2', '2', '2', '2', 0 };
    static const WCHAR fld_am1[] = { 'A', 0 };
    static const WCHAR fld_am2[] = { 'A', 'M', 0 };
    static const WCHAR fld_day[] = { 'W', 'e', 'd', 'n', 'e', 's', 'd', 'a', 'y', 0 };
    static const WCHAR fld_day3[] = { 'W', 'e', 'd', 0 };
    static const WCHAR fld_mon[] = { 'S', 'e', 'p', 't', 'e', 'm', 'b', 'e', 'r', 0 };
    static const WCHAR fld_mon3[] = { 'D', 'e', 'c', 0 };
    int spec;
    WCHAR buffer[80];
    LPCWSTR bufptr;
    SIZE size;

    TRACE ("%d,%d\n", infoPtr->nrFields, count);
    if (count>infoPtr->nrFields || count < 0) {
	WARN ("buffer overrun, have %d want %d\n", infoPtr->nrFields, count);
	return;
    }

    if (!infoPtr->fieldspec) return;

    spec = infoPtr->fieldspec[count];
    if (spec & DT_STRING) {
	int txtlen = infoPtr->buflen[count];

        if (txtlen > 79)
            txtlen = 79;
	memcpy (buffer, infoPtr->textbuf + (spec &~ DT_STRING), txtlen * sizeof(WCHAR));
	buffer[txtlen] = 0;
	bufptr = buffer;
    }
    else {
        switch (spec) {
	    case ONEDIGITDAY:
	    case ONEDIGIT12HOUR:
	    case ONEDIGIT24HOUR:
	    case ONEDIGITSECOND:
	    case ONEDIGITMINUTE:
	    case ONEDIGITMONTH:
	    case ONEDIGITYEAR:
	        /* these seem to use a two byte field */
	    case TWODIGITDAY:
	    case TWODIGIT12HOUR:
	    case TWODIGIT24HOUR:
	    case TWODIGITSECOND:
	    case TWODIGITMINUTE:
	    case TWODIGITMONTH:
	    case TWODIGITYEAR:
	        bufptr = fld_d2W;
	        break;
            case INVALIDFULLYEAR:
	    case FULLYEAR:
	        bufptr = fld_d4W;
	        break;
	    case THREECHARDAY:
	        bufptr = fld_day3;
	        break;
	    case FULLDAY:
	        bufptr = fld_day;
	        break;
	    case THREECHARMONTH:
	        bufptr = fld_mon3;
	        break;
	    case FULLMONTH:
	        bufptr = fld_mon;
	        break;
	    case ONELETTERAMPM:
	        bufptr = fld_am1;
	        break;
	    case TWOLETTERAMPM:
	        bufptr = fld_am2;
	        break;
	    default:
	        bufptr = fld_d1W;
	        break;
        }
    }
    GetTextExtentPoint32W (hdc, bufptr, strlenW(bufptr), &size);
    *fieldWidthPtr = size.cx;
}

static void 
DATETIME_Refresh (DATETIME_INFO *infoPtr, HDC hdc)
{
    int i,prevright;
    RECT *field;
    RECT *rcDraw = &infoPtr->rcDraw;
    RECT *calbutton = &infoPtr->calbutton;
    RECT *checkbox = &infoPtr->checkbox;
    SIZE size;
    COLORREF oldTextColor;
    SHORT fieldWidth = 0;

    /* draw control edge */
    TRACE("\n");

    if (infoPtr->dateValid) {
        HFONT oldFont = SelectObject (hdc, infoPtr->hFont);
        INT oldBkMode = SetBkMode (hdc, TRANSPARENT);
        WCHAR txt[80];

        DATETIME_ReturnTxt (infoPtr, 0, txt, sizeof(txt)/sizeof(txt[0]));
        GetTextExtentPoint32W (hdc, txt, strlenW(txt), &size);
        rcDraw->bottom = size.cy + 2;

        prevright = checkbox->right = ((infoPtr->dwStyle & DTS_SHOWNONE) ? 18 : 2);

        for (i = 0; i < infoPtr->nrFields; i++) {
            DATETIME_ReturnTxt (infoPtr, i, txt, sizeof(txt)/sizeof(txt[0]));
            GetTextExtentPoint32W (hdc, txt, strlenW(txt), &size);
            DATETIME_ReturnFieldWidth (infoPtr, hdc, i, &fieldWidth);
            field = &infoPtr->fieldRect[i];
            field->left  = prevright;
            field->right = prevright + fieldWidth;
            field->top   = rcDraw->top;
            field->bottom = rcDraw->bottom;
            prevright = field->right;

            if (infoPtr->dwStyle & WS_DISABLED)
                oldTextColor = SetTextColor (hdc, comctl32_color.clrGrayText);
            else if ((infoPtr->haveFocus) && (i == infoPtr->select)) {
                /* fill if focussed */
                HBRUSH hbr = CreateSolidBrush (comctl32_color.clrActiveCaption);
                FillRect(hdc, field, hbr);
                DeleteObject (hbr);
                oldTextColor = SetTextColor (hdc, comctl32_color.clrWindow);
            }
            else
                oldTextColor = SetTextColor (hdc, comctl32_color.clrWindowText);

            /* draw the date text using the colour set above */
            DrawTextW (hdc, txt, strlenW(txt), field, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
            SetTextColor (hdc, oldTextColor);
        }
        SetBkMode (hdc, oldBkMode);
        SelectObject (hdc, oldFont);
    }

    if (!(infoPtr->dwStyle & DTS_UPDOWN)) {
        DrawFrameControl(hdc, calbutton, DFC_SCROLL,
                         DFCS_SCROLLDOWN | (infoPtr->bCalDepressed ? DFCS_PUSHED : 0) |
                         (infoPtr->dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0) );
    }
}


static INT
DATETIME_HitTest (const DATETIME_INFO *infoPtr, POINT pt)
{
    int i;

    TRACE ("%d, %d\n", pt.x, pt.y);

    if (PtInRect (&infoPtr->calbutton, pt)) return DTHT_MCPOPUP;
    if (PtInRect (&infoPtr->checkbox, pt)) return DTHT_CHECKBOX;

    for (i=0; i < infoPtr->nrFields; i++) {
        if (PtInRect (&infoPtr->fieldRect[i], pt)) return i;
    }

    return DTHT_NONE;
}


static LRESULT
DATETIME_LButtonDown (DATETIME_INFO *infoPtr, WORD wKey, INT x, INT y)
{
    POINT pt;
    int old, new;

    pt.x = x;
    pt.y = y;
    old = infoPtr->select;
    new = DATETIME_HitTest (infoPtr, pt);

    /* FIXME: might be conditions where we don't want to update infoPtr->select */
    infoPtr->select = new;

    SetFocus(infoPtr->hwndSelf);

    if (infoPtr->select == DTHT_MCPOPUP) {
        RECT rcMonthCal;
        SendMessageW(infoPtr->hMonthCal, MCM_GETMINREQRECT, 0, (LPARAM)&rcMonthCal);

        /* FIXME: button actually is only depressed during dropdown of the */
        /* calendar control and when the mouse is over the button window */
        infoPtr->bCalDepressed = TRUE;

        /* recalculate the position of the monthcal popup */
        if(infoPtr->dwStyle & DTS_RIGHTALIGN)
            infoPtr->monthcal_pos.x = infoPtr->calbutton.left - 
                (rcMonthCal.right - rcMonthCal.left);
        else
            /* FIXME: this should be after the area reserved for the checkbox */
            infoPtr->monthcal_pos.x = infoPtr->rcDraw.left;

        infoPtr->monthcal_pos.y = infoPtr->rcClient.bottom;
        ClientToScreen (infoPtr->hwndSelf, &(infoPtr->monthcal_pos));
        SetWindowPos(infoPtr->hMonthCal, 0, infoPtr->monthcal_pos.x,
            infoPtr->monthcal_pos.y, rcMonthCal.right - rcMonthCal.left,
            rcMonthCal.bottom - rcMonthCal.top, 0);

        if(IsWindowVisible(infoPtr->hMonthCal)) {
            ShowWindow(infoPtr->hMonthCal, SW_HIDE);
        } else {
            const SYSTEMTIME *lprgSysTimeArray = &infoPtr->date;
            TRACE("update calendar %04d/%02d/%02d\n", 
            lprgSysTimeArray->wYear, lprgSysTimeArray->wMonth, lprgSysTimeArray->wDay);
            SendMessageW(infoPtr->hMonthCal, MCM_SETCURSEL, 0, (LPARAM)(&infoPtr->date));
            ShowWindow(infoPtr->hMonthCal, SW_SHOW);
        }

        TRACE ("dt:%p mc:%p mc parent:%p, desktop:%p\n",
               infoPtr->hwndSelf, infoPtr->hMonthCal, infoPtr->hwndNotify, GetDesktopWindow ());
        DATETIME_SendSimpleNotify (infoPtr, DTN_DROPDOWN);
    }

    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return 0;
}


static LRESULT
DATETIME_LButtonUp (DATETIME_INFO *infoPtr, WORD wKey)
{
    if(infoPtr->bCalDepressed) {
        infoPtr->bCalDepressed = FALSE;
        InvalidateRect(infoPtr->hwndSelf, &(infoPtr->calbutton), TRUE);
    }

    return 0;
}


static LRESULT
DATETIME_Paint (DATETIME_INFO *infoPtr, HDC hdc)
{
    if (!hdc) {
	PAINTSTRUCT ps;
        hdc = BeginPaint (infoPtr->hwndSelf, &ps);
        DATETIME_Refresh (infoPtr, hdc);
        EndPaint (infoPtr->hwndSelf, &ps);
    } else {
        DATETIME_Refresh (infoPtr, hdc);
    }
    return 0;
}


static LRESULT
DATETIME_Button_Command (DATETIME_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    if( HIWORD(wParam) == BN_CLICKED) {
        DWORD state = SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0);
        infoPtr->dateValid = (state == BST_CHECKED);
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    }
    return 0;
}
          
        
        
static LRESULT
DATETIME_Command (DATETIME_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwndbutton = %p\n", infoPtr->hwndCheckbut);
    if(infoPtr->hwndCheckbut == (HWND)lParam)
        return DATETIME_Button_Command(infoPtr, wParam, lParam);
    return 0;
}


static LRESULT
DATETIME_Enable (DATETIME_INFO *infoPtr, BOOL bEnable)
{
    TRACE("%p %s\n", infoPtr, bEnable ? "TRUE" : "FALSE");
    if (bEnable)
        infoPtr->dwStyle &= ~WS_DISABLED;
    else
        infoPtr->dwStyle |= WS_DISABLED;

    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return 0;
}


static LRESULT
DATETIME_EraseBackground (const DATETIME_INFO *infoPtr, HDC hdc)
{
    HBRUSH hBrush, hSolidBrush = NULL;
    RECT   rc;

    if (infoPtr->dwStyle & WS_DISABLED)
        hBrush = hSolidBrush = CreateSolidBrush(comctl32_color.clrBtnFace);
    else
    {
        hBrush = (HBRUSH)SendMessageW(infoPtr->hwndNotify, WM_CTLCOLOREDIT,
                                      (WPARAM)hdc, (LPARAM)infoPtr->hwndSelf);
        if (!hBrush)
            hBrush = hSolidBrush = CreateSolidBrush(comctl32_color.clrWindow);
    }

    GetClientRect (infoPtr->hwndSelf, &rc);

    FillRect (hdc, &rc, hBrush);

    if (hSolidBrush)
        DeleteObject(hSolidBrush);

    return -1;
}


static LRESULT
DATETIME_Notify (DATETIME_INFO *infoPtr, int idCtrl, LPNMHDR lpnmh)
{
    TRACE ("Got notification %x from %p\n", lpnmh->code, lpnmh->hwndFrom);
    TRACE ("info: %p %p %p\n", infoPtr->hwndSelf, infoPtr->hMonthCal, infoPtr->hUpdown);

    if (lpnmh->code == MCN_SELECT) {
        ShowWindow(infoPtr->hMonthCal, SW_HIDE);
        infoPtr->dateValid = TRUE;
        SendMessageW (infoPtr->hMonthCal, MCM_GETCURSEL, 0, (LPARAM)&infoPtr->date);
        TRACE("got from calendar %04d/%02d/%02d day of week %d\n", 
        infoPtr->date.wYear, infoPtr->date.wMonth, infoPtr->date.wDay, infoPtr->date.wDayOfWeek);
        SendMessageW (infoPtr->hwndCheckbut, BM_SETCHECK, BST_CHECKED, 0);
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
        DATETIME_SendDateTimeChangeNotify (infoPtr);
    }
    if ((lpnmh->hwndFrom == infoPtr->hUpdown) && (lpnmh->code == UDN_DELTAPOS)) {
        LPNMUPDOWN lpnmud = (LPNMUPDOWN)lpnmh;
        TRACE("Delta pos %d\n", lpnmud->iDelta);
        infoPtr->pendingUpdown = lpnmud->iDelta;
    }
    return 0;
}


static LRESULT
DATETIME_KeyDown (DATETIME_INFO *infoPtr, DWORD vkCode, LPARAM flags)
{
    int fieldNum = infoPtr->select & DTHT_DATEFIELD;
    int wrap = 0;

    if (!(infoPtr->haveFocus)) return 0;
    if ((fieldNum==0) && (infoPtr->select)) return 0;

    if (infoPtr->select & FORMATCALLMASK) {
	FIXME ("Callbacks not implemented yet\n");
    }

    if (vkCode >= '0' && vkCode <= '9') {
        /* this is a somewhat simplified version of what Windows does */
	SYSTEMTIME *date = &infoPtr->date;
	switch (infoPtr->fieldspec[fieldNum]) {
	    case ONEDIGITYEAR:
	    case TWODIGITYEAR:
	        date->wYear = date->wYear - (date->wYear%100) +
	                (date->wYear%10)*10 + (vkCode-'0');
	        date->wDayOfWeek = DATETIME_CalculateDayOfWeek(
	                date->wDay,date->wMonth,date->wYear);
	        DATETIME_SendDateTimeChangeNotify (infoPtr);
	        break;
            case INVALIDFULLYEAR:
	    case FULLYEAR:
	        date->wYear = (date->wYear%1000)*10 + (vkCode-'0');
	        date->wDayOfWeek = DATETIME_CalculateDayOfWeek(
	                date->wDay,date->wMonth,date->wYear);
	        DATETIME_SendDateTimeChangeNotify (infoPtr);
	        break;
	    case ONEDIGITMONTH:
	    case TWODIGITMONTH:
	        if ((date->wMonth%10) > 1 || (vkCode-'0') > 2)
	            date->wMonth = vkCode-'0';
	        else
	            date->wMonth = (date->wMonth%10)*10+vkCode-'0';
	        date->wDayOfWeek = DATETIME_CalculateDayOfWeek(
	                date->wDay,date->wMonth,date->wYear);
	        DATETIME_SendDateTimeChangeNotify (infoPtr);
	        break;
	    case ONEDIGITDAY:
	    case TWODIGITDAY:
	        /* probably better checking here would help */
	        if ((date->wDay%10) >= 3 && (vkCode-'0') > 1)
	            date->wDay = vkCode-'0';
	        else
	            date->wDay = (date->wDay%10)*10+vkCode-'0';
	        date->wDayOfWeek = DATETIME_CalculateDayOfWeek(
	                date->wDay,date->wMonth,date->wYear);
	        DATETIME_SendDateTimeChangeNotify (infoPtr);
	        break;
	    case ONEDIGIT12HOUR:
	    case TWODIGIT12HOUR:
	        if ((date->wHour%10) > 1 || (vkCode-'0') > 2)
	            date->wHour = vkCode-'0';
	        else
	            date->wHour = (date->wHour%10)*10+vkCode-'0';
	        DATETIME_SendDateTimeChangeNotify (infoPtr);
	        break;
	    case ONEDIGIT24HOUR:
	    case TWODIGIT24HOUR:
	        if ((date->wHour%10) > 2)
	            date->wHour = vkCode-'0';
	        else if ((date->wHour%10) == 2 && (vkCode-'0') > 3)
	            date->wHour = vkCode-'0';
	        else
	            date->wHour = (date->wHour%10)*10+vkCode-'0';
	        DATETIME_SendDateTimeChangeNotify (infoPtr);
	        break;
	    case ONEDIGITMINUTE:
	    case TWODIGITMINUTE:
	        if ((date->wMinute%10) > 5)
	            date->wMinute = vkCode-'0';
	        else
	            date->wMinute = (date->wMinute%10)*10+vkCode-'0';
	        DATETIME_SendDateTimeChangeNotify (infoPtr);
	        break;
	    case ONEDIGITSECOND:
	    case TWODIGITSECOND:
	        if ((date->wSecond%10) > 5)
	            date->wSecond = vkCode-'0';
	        else
	            date->wSecond = (date->wSecond%10)*10+vkCode-'0';
	        DATETIME_SendDateTimeChangeNotify (infoPtr);
	        break;
	}
    }
    
    switch (vkCode) {
	case VK_ADD:
    	case VK_UP:
	    DATETIME_IncreaseField (infoPtr, fieldNum, 1);
	    DATETIME_SendDateTimeChangeNotify (infoPtr);
	    break;
	case VK_SUBTRACT:
	case VK_DOWN:
	    DATETIME_IncreaseField (infoPtr, fieldNum, -1);
	    DATETIME_SendDateTimeChangeNotify (infoPtr);
	    break;
	case VK_HOME:
	    DATETIME_IncreaseField (infoPtr, fieldNum, INT_MIN);
	    DATETIME_SendDateTimeChangeNotify (infoPtr);
	    break;
	case VK_END:
	    DATETIME_IncreaseField (infoPtr, fieldNum, INT_MAX);
	    DATETIME_SendDateTimeChangeNotify (infoPtr);
	    break;
	case VK_LEFT:
	    do {
		if (infoPtr->select == 0) {
		    infoPtr->select = infoPtr->nrFields - 1;
		    wrap++;
		} else {
		    infoPtr->select--;
		}
	    } while ((infoPtr->fieldspec[infoPtr->select] & DT_STRING) && (wrap<2));
	    break;
	case VK_RIGHT:
	    do {
		infoPtr->select++;
		if (infoPtr->select==infoPtr->nrFields) {
		    infoPtr->select = 0;
		    wrap++;
		}
	    } while ((infoPtr->fieldspec[infoPtr->select] & DT_STRING) && (wrap<2));
	    break;
    }

    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return 0;
}


static LRESULT
DATETIME_VScroll (DATETIME_INFO *infoPtr, WORD wScroll)
{
    int fieldNum = infoPtr->select & DTHT_DATEFIELD;

    if ((SHORT)LOWORD(wScroll) != SB_THUMBPOSITION) return 0;
    if (!(infoPtr->haveFocus)) return 0;
    if ((fieldNum==0) && (infoPtr->select)) return 0;

    if (infoPtr->pendingUpdown >= 0) {
	DATETIME_IncreaseField (infoPtr, fieldNum, 1);
	DATETIME_SendDateTimeChangeNotify (infoPtr);
    }
    else {
	DATETIME_IncreaseField (infoPtr, fieldNum, -1);
	DATETIME_SendDateTimeChangeNotify (infoPtr);
    }

    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return 0;
}


static LRESULT
DATETIME_KillFocus (DATETIME_INFO *infoPtr, HWND lostFocus)
{
    TRACE("lost focus to %p\n", lostFocus);

    if (infoPtr->haveFocus) {
	DATETIME_SendSimpleNotify (infoPtr, NM_KILLFOCUS);
	infoPtr->haveFocus = 0;
    }

    InvalidateRect (infoPtr->hwndSelf, NULL, TRUE);

    return 0;
}


static LRESULT
DATETIME_NCCreate (HWND hwnd, const CREATESTRUCTW *lpcs)
{
    DWORD dwExStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    /* force control to have client edge */
    dwExStyle |= WS_EX_CLIENTEDGE;
    SetWindowLongW(hwnd, GWL_EXSTYLE, dwExStyle);

    return DefWindowProcW(hwnd, WM_NCCREATE, 0, (LPARAM)lpcs);
}


static LRESULT
DATETIME_SetFocus (DATETIME_INFO *infoPtr, HWND lostFocus)
{
    TRACE("got focus from %p\n", lostFocus);

    if (infoPtr->haveFocus == 0) {
	DATETIME_SendSimpleNotify (infoPtr, NM_SETFOCUS);
	infoPtr->haveFocus = DTHT_GOTFOCUS;
    }

    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static BOOL
DATETIME_SendDateTimeChangeNotify (const DATETIME_INFO *infoPtr)
{
    NMDATETIMECHANGE dtdtc;

    dtdtc.nmhdr.hwndFrom = infoPtr->hwndSelf;
    dtdtc.nmhdr.idFrom   = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
    dtdtc.nmhdr.code     = DTN_DATETIMECHANGE;

    dtdtc.dwFlags = (infoPtr->dwStyle & DTS_SHOWNONE) ? GDT_NONE : GDT_VALID;

    MONTHCAL_CopyTime (&infoPtr->date, &dtdtc.st);
    return (BOOL) SendMessageW (infoPtr->hwndNotify, WM_NOTIFY,
                                (WPARAM)dtdtc.nmhdr.idFrom, (LPARAM)&dtdtc);
}


static BOOL
DATETIME_SendSimpleNotify (const DATETIME_INFO *infoPtr, UINT code)
{
    NMHDR nmhdr;

    TRACE("%x\n", code);
    nmhdr.hwndFrom = infoPtr->hwndSelf;
    nmhdr.idFrom   = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
    nmhdr.code     = code;

    return (BOOL) SendMessageW (infoPtr->hwndNotify, WM_NOTIFY,
                                (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);
}

static LRESULT
DATETIME_Size (DATETIME_INFO *infoPtr, WORD flags, INT width, INT height)
{
    /* set size */
    infoPtr->rcClient.bottom = height;
    infoPtr->rcClient.right = width;

    TRACE("Height=%d, Width=%d\n", infoPtr->rcClient.bottom, infoPtr->rcClient.right);

    infoPtr->rcDraw = infoPtr->rcClient;
    
    if (infoPtr->dwStyle & DTS_UPDOWN) {
        SetWindowPos(infoPtr->hUpdown, NULL,
            infoPtr->rcClient.right-14, 0,
            15, infoPtr->rcClient.bottom - infoPtr->rcClient.top,
            SWP_NOACTIVATE | SWP_NOZORDER);
    }
    else {
        /* set the size of the button that drops the calendar down */
        /* FIXME: account for style that allows button on left side */
        infoPtr->calbutton.top   = infoPtr->rcDraw.top;
        infoPtr->calbutton.bottom= infoPtr->rcDraw.bottom;
        infoPtr->calbutton.left  = infoPtr->rcDraw.right-15;
        infoPtr->calbutton.right = infoPtr->rcDraw.right;
    }

    /* set enable/disable button size for show none style being enabled */
    /* FIXME: these dimensions are completely incorrect */
    infoPtr->checkbox.top = infoPtr->rcDraw.top;
    infoPtr->checkbox.bottom = infoPtr->rcDraw.bottom;
    infoPtr->checkbox.left = infoPtr->rcDraw.left;
    infoPtr->checkbox.right = infoPtr->rcDraw.left + 10;

    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static LRESULT 
DATETIME_StyleChanged(DATETIME_INFO *infoPtr, WPARAM wStyleType, const STYLESTRUCT *lpss)
{
    static const WCHAR buttonW[] = { 'b', 'u', 't', 't', 'o', 'n', 0 };

    TRACE("(styletype=%lx, styleOld=0x%08x, styleNew=0x%08x)\n",
          wStyleType, lpss->styleOld, lpss->styleNew);

    if (wStyleType != GWL_STYLE) return 0;
  
    infoPtr->dwStyle = lpss->styleNew;

    if ( !(lpss->styleOld & DTS_SHOWNONE) && (lpss->styleNew & DTS_SHOWNONE) ) {
        infoPtr->hwndCheckbut = CreateWindowExW (0, buttonW, 0, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
         					 2, 2, 13, 13, infoPtr->hwndSelf, 0, 
						(HINSTANCE)GetWindowLongPtrW (infoPtr->hwndSelf, GWLP_HINSTANCE), 0);
        SendMessageW (infoPtr->hwndCheckbut, BM_SETCHECK, 1, 0);
    }
    if ( (lpss->styleOld & DTS_SHOWNONE) && !(lpss->styleNew & DTS_SHOWNONE) ) {
        DestroyWindow(infoPtr->hwndCheckbut);
        infoPtr->hwndCheckbut = 0;
    }
    if ( !(lpss->styleOld & DTS_UPDOWN) && (lpss->styleNew & DTS_UPDOWN) ) {
	infoPtr->hUpdown = CreateUpDownControl (WS_CHILD | WS_BORDER | WS_VISIBLE, 120, 1, 20, 20, 
						infoPtr->hwndSelf, 1, 0, 0, UD_MAXVAL, UD_MINVAL, 0);
    }
    if ( (lpss->styleOld & DTS_UPDOWN) && !(lpss->styleNew & DTS_UPDOWN) ) {
	DestroyWindow(infoPtr->hUpdown);
	infoPtr->hUpdown = 0;
    }

    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return 0;
}


static LRESULT
DATETIME_SetFont (DATETIME_INFO *infoPtr, HFONT font, BOOL repaint)
{
    infoPtr->hFont = font;
    if (repaint) InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return 0;
}


static LRESULT
DATETIME_Create (HWND hwnd, const CREATESTRUCTW *lpcs)
{
    static const WCHAR SysMonthCal32W[] = { 'S', 'y', 's', 'M', 'o', 'n', 't', 'h', 'C', 'a', 'l', '3', '2', 0 };
    DATETIME_INFO *infoPtr = (DATETIME_INFO *)Alloc (sizeof(DATETIME_INFO));
    STYLESTRUCT ss = { 0, lpcs->style };

    if (!infoPtr) return -1;

    infoPtr->hwndSelf = hwnd;
    infoPtr->dwStyle = lpcs->style;

    infoPtr->nrFieldsAllocated = 32;
    infoPtr->fieldspec = (int *) Alloc (infoPtr->nrFieldsAllocated * sizeof(int));
    infoPtr->fieldRect = (RECT *) Alloc (infoPtr->nrFieldsAllocated * sizeof(RECT));
    infoPtr->buflen = (int *) Alloc (infoPtr->nrFieldsAllocated * sizeof(int));
    infoPtr->hwndNotify = lpcs->hwndParent;
    infoPtr->select = -1; /* initially, nothing is selected */

    DATETIME_StyleChanged(infoPtr, GWL_STYLE, &ss);
    DATETIME_SetFormatW (infoPtr, 0);

    /* create the monthcal control */
    infoPtr->hMonthCal = CreateWindowExW (0, SysMonthCal32W, 0, WS_BORDER | WS_POPUP | WS_CLIPSIBLINGS, 
					  0, 0, 0, 0, infoPtr->hwndSelf, 0, 0, 0);

    /* initialize info structure */
    GetLocalTime (&infoPtr->date);
    infoPtr->dateValid = TRUE;
    infoPtr->hFont = GetStockObject(DEFAULT_GUI_FONT);

    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

    return 0;
}



static LRESULT
DATETIME_Destroy (DATETIME_INFO *infoPtr)
{
    if (infoPtr->hwndCheckbut)
	DestroyWindow(infoPtr->hwndCheckbut);
    if (infoPtr->hUpdown)
	DestroyWindow(infoPtr->hUpdown);
    if (infoPtr->hMonthCal) 
        DestroyWindow(infoPtr->hMonthCal);
    SetWindowLongPtrW( infoPtr->hwndSelf, 0, 0 ); /* clear infoPtr */
    Free (infoPtr);
    return 0;
}


static LRESULT WINAPI
DATETIME_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr = ((DATETIME_INFO *)GetWindowLongPtrW (hwnd, 0));
    LRESULT ret;

    TRACE ("%x, %lx, %lx\n", uMsg, wParam, lParam);

    if (!infoPtr && (uMsg != WM_CREATE) && (uMsg != WM_NCCREATE))
	return DefWindowProcW( hwnd, uMsg, wParam, lParam );

    switch (uMsg) {

    case DTM_GETSYSTEMTIME:
        return DATETIME_GetSystemTime (infoPtr, (SYSTEMTIME *) lParam);

    case DTM_SETSYSTEMTIME:
	return DATETIME_SetSystemTime (infoPtr, wParam, (SYSTEMTIME *) lParam);

    case DTM_GETRANGE:
	ret = SendMessageW (infoPtr->hMonthCal, MCM_GETRANGE, wParam, lParam);
	return ret ? ret : 1; /* bug emulation */

    case DTM_SETRANGE:
	return SendMessageW (infoPtr->hMonthCal, MCM_SETRANGE, wParam, lParam);

    case DTM_SETFORMATA:
        return DATETIME_SetFormatA (infoPtr, (LPCSTR)lParam);

    case DTM_SETFORMATW:
        return DATETIME_SetFormatW (infoPtr, (LPCWSTR)lParam);

    case DTM_GETMONTHCAL:
	return (LRESULT)infoPtr->hMonthCal;

    case DTM_SETMCCOLOR:
	return SendMessageW (infoPtr->hMonthCal, MCM_SETCOLOR, wParam, lParam);

    case DTM_GETMCCOLOR:
        return SendMessageW (infoPtr->hMonthCal, MCM_GETCOLOR, wParam, 0);

    case DTM_SETMCFONT:
	return SendMessageW (infoPtr->hMonthCal, WM_SETFONT, wParam, lParam);

    case DTM_GETMCFONT:
	return SendMessageW (infoPtr->hMonthCal, WM_GETFONT, wParam, lParam);

    case WM_NOTIFY:
	return DATETIME_Notify (infoPtr, (int)wParam, (LPNMHDR)lParam);

    case WM_ENABLE:
        return DATETIME_Enable (infoPtr, (BOOL)wParam);

    case WM_ERASEBKGND:
        return DATETIME_EraseBackground (infoPtr, (HDC)wParam);

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;

    case WM_PRINTCLIENT:
    case WM_PAINT:
        return DATETIME_Paint (infoPtr, (HDC)wParam);

    case WM_KEYDOWN:
        return DATETIME_KeyDown (infoPtr, wParam, lParam);

    case WM_KILLFOCUS:
        return DATETIME_KillFocus (infoPtr, (HWND)wParam);

    case WM_NCCREATE:
        return DATETIME_NCCreate (hwnd, (LPCREATESTRUCTW)lParam);

    case WM_SETFOCUS:
        return DATETIME_SetFocus (infoPtr, (HWND)wParam);

    case WM_SIZE:
        return DATETIME_Size (infoPtr, wParam, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

    case WM_LBUTTONDOWN:
        return DATETIME_LButtonDown (infoPtr, (WORD)wParam, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

    case WM_LBUTTONUP:
        return DATETIME_LButtonUp (infoPtr, (WORD)wParam);

    case WM_VSCROLL:
        return DATETIME_VScroll (infoPtr, (WORD)wParam);

    case WM_CREATE:
	return DATETIME_Create (hwnd, (LPCREATESTRUCTW)lParam);

    case WM_DESTROY:
	return DATETIME_Destroy (infoPtr);

    case WM_COMMAND:
        return DATETIME_Command (infoPtr, wParam, lParam);

    case WM_STYLECHANGED:
        return DATETIME_StyleChanged(infoPtr, wParam, (LPSTYLESTRUCT)lParam);

    case WM_SETFONT:
        return DATETIME_SetFont(infoPtr, (HFONT)wParam, (BOOL)lParam);

    case WM_GETFONT:
        return (LRESULT) infoPtr->hFont;

    default:
	if ((uMsg >= WM_USER) && (uMsg < WM_APP))
		ERR("unknown msg %04x wp=%08lx lp=%08lx\n",
		     uMsg, wParam, lParam);
	return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    }
}


void
DATETIME_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = DATETIME_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(DATETIME_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPCWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = DATETIMEPICK_CLASSW;

    RegisterClassW (&wndClass);
}


void
DATETIME_Unregister (void)
{
    UnregisterClassW (DATETIMEPICK_CLASSW, NULL);
}
