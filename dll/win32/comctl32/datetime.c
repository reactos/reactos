/*
 * Date and time picker control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 1999, 2000 Alex Priem <alexp@sci.kun.nl>
 * Copyright 2000 Chris Morgan <cmorgan@wpi.edu>
 * Copyright 2012 Owen Rudge for CodeWeavers
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
 *    -- DTS_APPCANPARSE
 *    -- DTS_SHORTDATECENTURYFORMAT
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
#include "uxtheme.h"
#include "vsstyle.h"
#include "wine/debug.h"

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
    BOOL bCalHot; /* TRUE if calendar button is hovered */
    BOOL bDropdownEnabled;
    int  select;
    WCHAR charsEntered[4];
    int nCharsEntered;
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

#define DTHT_NONE       0x1000
#define DTHT_CHECKBOX   0x2000  /* these should end at '00' , to make */
#define DTHT_MCPOPUP    0x3000  /* & DTHT_DATEFIELD 0 when DATETIME_KeyDown */
#define DTHT_GOTFOCUS   0x4000  /* tests for date-fields */
#define DTHT_NODATEMASK 0xf000  /* to mask check and drop down from others */

static BOOL DATETIME_SendSimpleNotify (const DATETIME_INFO *infoPtr, UINT code);
static BOOL DATETIME_SendDateTimeChangeNotify (const DATETIME_INFO *infoPtr);
static const WCHAR allowedformatchars[] = L"dhHmMstyX";
static const int maxrepetition [] = {4,2,2,2,4,2,2,4,-1};
static const WCHAR *themeClass = WC_SCROLLBARW;

/* valid date limits */
static const SYSTEMTIME max_allowed_date = { .wYear = 9999, .wMonth = 12, .wDayOfWeek = 0, .wDay = 31 };
static const SYSTEMTIME min_allowed_date = { .wYear = 1752, .wMonth = 9, .wDayOfWeek = 0, .wDay = 14 };

static DWORD
DATETIME_GetSystemTime (const DATETIME_INFO *infoPtr, SYSTEMTIME *systime)
{
    if (!systime) return GDT_NONE;

    if ((infoPtr->dwStyle & DTS_SHOWNONE) &&
        (SendMessageW (infoPtr->hwndCheckbut, BM_GETCHECK, 0, 0) == BST_UNCHECKED))
        return GDT_NONE;

    *systime = infoPtr->date;

    return GDT_VALID;
}

/* Checks value is within configured date range
 *
 * PARAMETERS
 *
 *  [I] infoPtr : valid pointer to control data
 *  [I] date    : pointer to valid date data to check
 *
 * RETURN VALUE
 *
 *  TRUE  - date within configured range
 *  FALSE - date is outside configured range
 */
static BOOL DATETIME_IsDateInValidRange(const DATETIME_INFO *infoPtr, const SYSTEMTIME *date)
{
    SYSTEMTIME range[2];
    DWORD limits;

    if ((MONTHCAL_CompareSystemTime(date, &max_allowed_date) == 1) ||
        (MONTHCAL_CompareSystemTime(date, &min_allowed_date) == -1))
        return FALSE;

    limits = SendMessageW (infoPtr->hMonthCal, MCM_GETRANGE, 0, (LPARAM)range);

    if (limits & GDTR_MAX)
    {
        if (MONTHCAL_CompareSystemTime(date, &range[1]) == 1)
           return FALSE;
    }

    if (limits & GDTR_MIN)
    {
        if (MONTHCAL_CompareSystemTime(date, &range[0]) == -1)
           return FALSE;
    }

    return TRUE;
}

static BOOL
DATETIME_SetSystemTime (DATETIME_INFO *infoPtr, DWORD flag, const SYSTEMTIME *systime)
{
    if (!systime) return FALSE;

    TRACE("%04d/%02d/%02d %02d:%02d:%02d\n",
          systime->wYear, systime->wMonth, systime->wDay,
          systime->wHour, systime->wMinute, systime->wSecond);

    if (flag == GDT_VALID) {
        if (systime->wYear == 0 ||
            systime->wMonth < 1 || systime->wMonth > 12 ||
            systime->wDay < 1 ||
            systime->wDay > MONTHCAL_MonthLength(systime->wMonth, systime->wYear) ||
            systime->wHour > 23 ||
            systime->wMinute > 59 ||
            systime->wSecond > 59 ||
            systime->wMilliseconds > 999
           )
            return FALSE;

        /* Windows returns true if the date is valid but outside the limits set */
        if (!DATETIME_IsDateInValidRange(infoPtr, systime))
            return TRUE;

        infoPtr->dateValid = TRUE;
        infoPtr->date = *systime;
        /* always store a valid day of week */
        MONTHCAL_CalculateDayOfWeek(&infoPtr->date, TRUE);

        SendMessageW (infoPtr->hMonthCal, MCM_SETCURSEL, 0, (LPARAM)(&infoPtr->date));
        SendMessageW (infoPtr->hwndCheckbut, BM_SETCHECK, BST_CHECKED, 0);
    } else if ((infoPtr->dwStyle & DTS_SHOWNONE) && (flag == GDT_NONE)) {
        infoPtr->dateValid = FALSE;
        SendMessageW (infoPtr->hwndCheckbut, BM_SETCHECK, BST_UNCHECKED, 0);
    }
    else
        return FALSE;

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
 * Therefore, 'string' ends up as '<show seconds>tring'.
 *
 */
static void
DATETIME_UseFormat (DATETIME_INFO *infoPtr, LPCWSTR formattxt)
{
    unsigned int i;
    int j, k, len;
    BOOL inside_literal = FALSE; /* inside '...' */
    int *nrFields = &infoPtr->nrFields;

    *nrFields = 0;
    infoPtr->fieldspec[*nrFields] = 0;
    len = lstrlenW(allowedformatchars);
    k = 0;

    for (i = 0; formattxt[i]; i++)  {
	TRACE ("\n%d %c:", i, formattxt[i]);
	if (!inside_literal) {
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
        }
        else
            j = len;

        if (formattxt[i] == '\'')
        {
            inside_literal = !inside_literal;
            continue;
        }

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
DATETIME_SetFormatW (DATETIME_INFO *infoPtr, LPCWSTR format)
{
    WCHAR format_buf[80];

    if (!format) {
	DWORD format_item;

        if ((infoPtr->dwStyle & DTS_SHORTDATECENTURYFORMAT) == DTS_SHORTDATECENTURYFORMAT)
            format_item = LOCALE_SSHORTDATE;
        else if ((infoPtr->dwStyle & DTS_LONGDATEFORMAT) == DTS_LONGDATEFORMAT)
            format_item = LOCALE_SLONGDATE;
        else if ((infoPtr->dwStyle & DTS_TIMEFORMAT) == DTS_TIMEFORMAT)
            format_item = LOCALE_STIMEFORMAT;
        else /* DTS_SHORTDATEFORMAT */
	    format_item = LOCALE_SSHORTDATE;
	GetLocaleInfoW(LOCALE_USER_DEFAULT, format_item, format_buf, ARRAY_SIZE(format_buf));
	format = format_buf;
    }

    DATETIME_UseFormat (infoPtr, format);
    InvalidateRect (infoPtr->hwndSelf, NULL, TRUE);

    return TRUE;
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
	    wsprintfW (result, L"%d", date.wDay);
	    break;
	case TWODIGITDAY:
	    wsprintfW (result, L"%.2d", date.wDay);
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
	        wsprintfW (result, L"%d", date.wHour - (date.wHour > 12 ? 12 : 0));
	    break;
	case TWODIGIT12HOUR:
	    if (date.wHour == 0) {
	        result[0] = '1';
	        result[1] = '2';
	        result[2] = 0;
	    }
	    else
	        wsprintfW (result, L"%.2d", date.wHour - (date.wHour > 12 ? 12 : 0));
	    break;
	case ONEDIGIT24HOUR:
	    wsprintfW (result, L"%d", date.wHour);
	    break;
	case TWODIGIT24HOUR:
	    wsprintfW (result, L"%.2d", date.wHour);
	    break;
	case ONEDIGITSECOND:
	    wsprintfW (result, L"%d", date.wSecond);
	    break;
	case TWODIGITSECOND:
	    wsprintfW (result, L"%.2d", date.wSecond);
	    break;
	case ONEDIGITMINUTE:
	    wsprintfW (result, L"%d", date.wMinute);
	    break;
	case TWODIGITMINUTE:
	    wsprintfW (result, L"%.2d", date.wMinute);
	    break;
	case ONEDIGITMONTH:
	    wsprintfW (result, L"%d", date.wMonth);
	    break;
	case TWODIGITMONTH:
	    wsprintfW (result, L"%.2d", date.wMonth);
	    break;
	case THREECHARMONTH:
	    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHNAME1+date.wMonth -1, buffer, ARRAY_SIZE(buffer));
	    wsprintfW (result, L"%s.3s", buffer);
	    break;
	case FULLMONTH:
	    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHNAME1+date.wMonth -1,
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
	    wsprintfW (result, L"%d", date.wYear % 10);
	    break;
	case TWODIGITYEAR:
	    wsprintfW (result, L"%.2d", date.wYear % 100);
	    break;
        case INVALIDFULLYEAR:
	case FULLYEAR:
	    wsprintfW (result, L"%d", date.wYear);
	    break;
    }

    TRACE ("arg%d=%x->[%s]\n", count, infoPtr->fieldspec[count], debugstr_w(result));
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
    SYSTEMTIME range[2];
    DWORD limits;
    BOOL min;

    TRACE ("%d\n", number);
    if ((number > infoPtr->nrFields) || (number < 0)) return;

    if ((infoPtr->fieldspec[number] & DTHT_DATEFIELD) == 0) return;

    switch (infoPtr->fieldspec[number]) {
	case ONEDIGITYEAR:
	case TWODIGITYEAR:
	case FULLYEAR:
            if (delta == INT_MIN)
                date->wYear = 1752;
            else if (delta == INT_MAX)
                date->wYear = 9999;
            else
                date->wYear = max(min(date->wYear + delta, 9999), 1752);

	    if (date->wDay > MONTHCAL_MonthLength(date->wMonth, date->wYear))
	        /* This can happen when moving away from a leap year. */
	        date->wDay = MONTHCAL_MonthLength(date->wMonth, date->wYear);
	    MONTHCAL_CalculateDayOfWeek(date, TRUE);
	    break;
	case ONEDIGITMONTH:
	case TWODIGITMONTH:
	case THREECHARMONTH:
	case FULLMONTH:
	    date->wMonth = wrap(date->wMonth, delta, 1, 12);
	    MONTHCAL_CalculateDayOfWeek(date, TRUE);
	    delta = 0;
	    /* fall through */
	case ONEDIGITDAY:
	case TWODIGITDAY:
	case THREECHARDAY:
	case FULLDAY:
	    date->wDay = wrap(date->wDay, delta, 1, MONTHCAL_MonthLength(date->wMonth, date->wYear));
	    MONTHCAL_CalculateDayOfWeek(date, TRUE);
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

    /* Ensure time is within bounds */
    limits = SendMessageW (infoPtr->hMonthCal, MCM_GETRANGE, 0, (LPARAM)range);
    min = delta < 0;

    if (limits & (min ? GDTR_MIN : GDTR_MAX))
    {
        int i = (min ? 0 : 1);

        if (MONTHCAL_CompareSystemTime(date, &range[i]) == (min ? -1 : 1))
        {
            date->wYear = range[i].wYear;
            date->wMonth = range[i].wMonth;
            date->wDayOfWeek = range[i].wDayOfWeek;
            date->wDay = range[i].wDay;
            date->wHour = range[i].wHour;
            date->wMinute = range[i].wMinute;
            date->wSecond = range[i].wSecond;
            date->wMilliseconds = range[i].wMilliseconds;
        }
    }
}

static int DATETIME_GetFieldWidth (const DATETIME_INFO *infoPtr, HDC hdc, int count)
{
    /* fields are a fixed width, determined by the largest possible string */
    /* presumably, these widths should be language dependent */
    int spec;
    WCHAR buffer[80];
    LPCWSTR bufptr;
    SIZE size;

    if (!infoPtr->fieldspec) return 0;

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
	        bufptr = L"22";
	        break;
            case INVALIDFULLYEAR:
	    case FULLYEAR:
	        bufptr = L"2222";
	        break;
	    case THREECHARMONTH:
	    case FULLMONTH:
	    case THREECHARDAY:
	    case FULLDAY:
	    {
		const WCHAR *fall;
		LCTYPE lctype;
		INT i, max_count;
		LONG cx;

		/* choose locale data type and fallback string */
		switch (spec) {
		case THREECHARDAY:
		    fall   = L"Wed";
		    lctype = LOCALE_SABBREVDAYNAME1;
		    max_count = 7;
		    break;
		case FULLDAY:
		    fall   = L"Wednesday";
		    lctype = LOCALE_SDAYNAME1;
		    max_count = 7;
		    break;
		case THREECHARMONTH:
		    fall   = L"Dec";
		    lctype = LOCALE_SABBREVMONTHNAME1;
		    max_count = 12;
		    break;
		default: /* FULLMONTH */
		    fall   = L"September";
		    lctype = LOCALE_SMONTHNAME1;
		    max_count = 12;
		    break;
		}

		cx = 0;
		for (i = 0; i < max_count; i++)
		{
		    if(GetLocaleInfoW(LOCALE_USER_DEFAULT, lctype + i,
			buffer, ARRAY_SIZE(buffer)))
		    {
			GetTextExtentPoint32W(hdc, buffer, lstrlenW(buffer), &size);
			if (size.cx > cx) cx = size.cx;
		    }
		    else /* locale independent fallback on failure */
		    {
		        GetTextExtentPoint32W(hdc, fall, lstrlenW(fall), &size);
			cx = size.cx;
		        break;
		    }
		}
		return cx;
	    }
	    case ONELETTERAMPM:
	        bufptr = L"A";
	        break;
	    case TWOLETTERAMPM:
	        bufptr = L"AM";
	        break;
	    default:
	        bufptr = L"2";
	        break;
        }
    }
    GetTextExtentPoint32W (hdc, bufptr, lstrlenW(bufptr), &size);
    return size.cx;
}

static void 
DATETIME_Refresh (DATETIME_INFO *infoPtr, HDC hdc)
{
    HTHEME theme;
    int state;

    TRACE("\n");

    if (infoPtr->dateValid) {
        int i, prevright;
        RECT *field;
        RECT *rcDraw = &infoPtr->rcDraw;
        SIZE size;
        COLORREF oldTextColor;
        HFONT oldFont = SelectObject (hdc, infoPtr->hFont);
        INT oldBkMode = SetBkMode (hdc, TRANSPARENT);
        WCHAR txt[80];

        DATETIME_ReturnTxt (infoPtr, 0, txt, ARRAY_SIZE(txt));
        GetTextExtentPoint32W (hdc, txt, lstrlenW(txt), &size);
        rcDraw->bottom = size.cy + 2;

        prevright = infoPtr->checkbox.right = ((infoPtr->dwStyle & DTS_SHOWNONE) ? 18 : 2);

        for (i = 0; i < infoPtr->nrFields; i++) {
            DATETIME_ReturnTxt (infoPtr, i, txt, ARRAY_SIZE(txt));
            GetTextExtentPoint32W (hdc, txt, lstrlenW(txt), &size);
            field = &infoPtr->fieldRect[i];
            field->left   = prevright;
            field->right  = prevright + DATETIME_GetFieldWidth (infoPtr, hdc, i);
            field->top    = rcDraw->top;
            field->bottom = rcDraw->bottom;
            prevright = field->right;

            if (infoPtr->dwStyle & WS_DISABLED)
                oldTextColor = SetTextColor (hdc, comctl32_color.clrGrayText);
            else if ((infoPtr->haveFocus) && (i == infoPtr->select)) {
                RECT selection;

                /* fill if focused */
                HBRUSH hbr = CreateSolidBrush (comctl32_color.clrActiveCaption);

                if (infoPtr->nCharsEntered)
                {
                    memcpy(txt, infoPtr->charsEntered, infoPtr->nCharsEntered * sizeof(WCHAR));
                    txt[infoPtr->nCharsEntered] = 0;
                    GetTextExtentPoint32W (hdc, txt, lstrlenW(txt), &size);
                }

                SetRect(&selection, 0, 0, size.cx, size.cy);
                /* center rectangle */
                OffsetRect(&selection, (field->right  + field->left - size.cx)/2,
                                       (field->bottom - size.cy)/2);

                FillRect(hdc, &selection, hbr);
                DeleteObject (hbr);
                oldTextColor = SetTextColor (hdc, comctl32_color.clrWindow);
            }
            else
                oldTextColor = SetTextColor (hdc, comctl32_color.clrWindowText);

            /* draw the date text using the colour set above */
            DrawTextW (hdc, txt, lstrlenW(txt), field, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SetTextColor (hdc, oldTextColor);
        }
        SetBkMode (hdc, oldBkMode);
        SelectObject (hdc, oldFont);
    }

    if (infoPtr->dwStyle & DTS_UPDOWN)
        return;

    theme = GetWindowTheme(infoPtr->hwndSelf);
    if (theme)
    {
        if (infoPtr->dwStyle & WS_DISABLED)
            state = ABS_DOWNDISABLED;
        else if (infoPtr->bCalDepressed)
            state = ABS_DOWNPRESSED;
        else if (infoPtr->bCalHot)
            state = ABS_DOWNHOT;
        else
            state = ABS_DOWNNORMAL;

        DrawThemeBackground(theme, hdc, SBP_ARROWBTN, state, &infoPtr->calbutton, NULL);
    }
    else
    {
        DrawFrameControl(hdc, &infoPtr->calbutton, DFC_SCROLL,
                         DFCS_SCROLLDOWN | (infoPtr->bCalDepressed ? DFCS_PUSHED : 0) |
                         (infoPtr->dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0) );
    }
}


static INT
DATETIME_HitTest (const DATETIME_INFO *infoPtr, POINT pt)
{
    int i;

    TRACE ("%s\n", wine_dbgstr_point(&pt));

    if (PtInRect (&infoPtr->calbutton, pt)) return DTHT_MCPOPUP;
    if (PtInRect (&infoPtr->checkbox, pt)) return DTHT_CHECKBOX;

    for (i = 0; i < infoPtr->nrFields; i++) {
        if (PtInRect (&infoPtr->fieldRect[i], pt)) return i;
    }

    return DTHT_NONE;
}

/* Returns index of the nearest preceding date field from given,
   or -1 if none was found */
static int DATETIME_GetPrevDateField(const DATETIME_INFO *infoPtr, int i)
{
    for(--i; i >= 0; i--)
    {
        if (infoPtr->fieldspec[i] & DTHT_DATEFIELD) return i;
    }
    return -1;
}

static void
DATETIME_ApplySelectedField (DATETIME_INFO *infoPtr)
{
    int fieldNum = infoPtr->select & DTHT_DATEFIELD;
    int i, val = 0;
    BOOL clamp_day = FALSE;
    SYSTEMTIME date = infoPtr->date;
    int oldyear;

    if (infoPtr->select == -1 || infoPtr->nCharsEntered == 0)
        return;

    if ((infoPtr->fieldspec[fieldNum] == ONELETTERAMPM) ||
        (infoPtr->fieldspec[fieldNum] == TWOLETTERAMPM))
        val = infoPtr->charsEntered[0];
    else {
        for (i=0; i<infoPtr->nCharsEntered; i++)
            val = val * 10 + infoPtr->charsEntered[i] - '0';
    }

    infoPtr->nCharsEntered = 0;

    switch (infoPtr->fieldspec[fieldNum]) {
        case ONEDIGITYEAR:
        case TWODIGITYEAR:
            oldyear = date.wYear;
            date.wYear = date.wYear - (date.wYear%100) + val;

            if (DATETIME_IsDateInValidRange(infoPtr, &date))
                clamp_day = TRUE;
            else
                date.wYear = oldyear;

            break;
        case INVALIDFULLYEAR:
        case FULLYEAR:
            oldyear = date.wYear;
            date.wYear = val;

            if (DATETIME_IsDateInValidRange(infoPtr, &date))
                clamp_day = TRUE;
            else
                date.wYear = oldyear;

            break;
        case ONEDIGITMONTH:
        case TWODIGITMONTH:
            date.wMonth = val;
            clamp_day = TRUE;
            break;
        case ONEDIGITDAY:
        case TWODIGITDAY:
            date.wDay = val;
            break;
        case ONEDIGIT12HOUR:
        case TWODIGIT12HOUR:
            if (val >= 24)
                val -= 20;

            if (val >= 13)
                date.wHour = val;
            else if (val != 0) {
                if (date.wHour >= 12) /* preserve current AM/PM state */
                    date.wHour = (val == 12 ? 12 : val + 12);
                else
                    date.wHour = (val == 12 ? 0 : val);
            }
            break;
        case ONEDIGIT24HOUR:
        case TWODIGIT24HOUR:
            date.wHour = val;
            break;
        case ONEDIGITMINUTE:
        case TWODIGITMINUTE:
            date.wMinute = val;
            break;
        case ONEDIGITSECOND:
        case TWODIGITSECOND:
            date.wSecond = val;
            break;
        case ONELETTERAMPM:
        case TWOLETTERAMPM:
            if (val == 'a' || val == 'A') {
                if (date.wHour >= 12)
                    date.wHour -= 12;
            } else if (val == 'p' || val == 'P') {
                if (date.wHour < 12)
                    date.wHour += 12;
            }
            break;
    }

    if (clamp_day && date.wDay > MONTHCAL_MonthLength(date.wMonth, date.wYear))
        date.wDay = MONTHCAL_MonthLength(date.wMonth, date.wYear);

    if (DATETIME_SetSystemTime(infoPtr, GDT_VALID, &date))
        DATETIME_SendDateTimeChangeNotify (infoPtr);
}

static void
DATETIME_SetSelectedField (DATETIME_INFO *infoPtr, int select)
{
    DATETIME_ApplySelectedField(infoPtr);

    infoPtr->select = select;
    infoPtr->nCharsEntered = 0;
}

static LRESULT
DATETIME_LButtonDown (DATETIME_INFO *infoPtr, INT x, INT y)
{
    POINT pt;
    int new;

    pt.x = x;
    pt.y = y;
    new = DATETIME_HitTest (infoPtr, pt);

    SetFocus(infoPtr->hwndSelf);

    if (!(new & DTHT_NODATEMASK) || (new == DTHT_NONE))
    {
        if (new == DTHT_NONE)
            new = infoPtr->nrFields - 1;
        else
        {
            /* hitting string part moves selection to next date field to left */
            if (infoPtr->fieldspec[new] & DT_STRING)
            {
                new = DATETIME_GetPrevDateField(infoPtr, new);
                if (new == -1) return 0;
            }
            /* never select full day of week */
            if (infoPtr->fieldspec[new] == FULLDAY) return 0;
        }
    }

    DATETIME_SetSelectedField(infoPtr, new);

    if (infoPtr->select == DTHT_MCPOPUP) {
        RECT rcMonthCal;
        POINT pos;
        SendMessageW(infoPtr->hMonthCal, MCM_GETMINREQRECT, 0, (LPARAM)&rcMonthCal);

        /* FIXME: button actually is only depressed during dropdown of the */
        /* calendar control and when the mouse is over the button window */
        infoPtr->bCalDepressed = TRUE;

        /* recalculate the position of the monthcal popup */
        if(infoPtr->dwStyle & DTS_RIGHTALIGN)
            pos.x = infoPtr->calbutton.left - (rcMonthCal.right - rcMonthCal.left);
        else
            /* FIXME: this should be after the area reserved for the checkbox */
            pos.x = infoPtr->rcDraw.left;

        pos.y = infoPtr->rcClient.bottom;
        OffsetRect( &rcMonthCal, pos.x, pos.y );
        MapWindowPoints( infoPtr->hwndSelf, 0, (POINT *)&rcMonthCal, 2 );
        SetWindowPos(infoPtr->hMonthCal, 0, rcMonthCal.left, rcMonthCal.top,
                     rcMonthCal.right - rcMonthCal.left, rcMonthCal.bottom - rcMonthCal.top, 0);

        if(IsWindowVisible(infoPtr->hMonthCal)) {
            ShowWindow(infoPtr->hMonthCal, SW_HIDE);
            infoPtr->bDropdownEnabled = FALSE;
            DATETIME_SendSimpleNotify (infoPtr, DTN_CLOSEUP);
        } else {
            const SYSTEMTIME *lprgSysTimeArray = &infoPtr->date;
            TRACE("update calendar %04d/%02d/%02d\n", 
            lprgSysTimeArray->wYear, lprgSysTimeArray->wMonth, lprgSysTimeArray->wDay);
            SendMessageW(infoPtr->hMonthCal, MCM_SETCURSEL, 0, (LPARAM)(&infoPtr->date));

            if (infoPtr->bDropdownEnabled) {
                ShowWindow(infoPtr->hMonthCal, SW_SHOW);
                DATETIME_SendSimpleNotify (infoPtr, DTN_DROPDOWN);
            }
            infoPtr->bDropdownEnabled = TRUE;
        }

        TRACE ("dt:%p mc:%p mc parent:%p, desktop:%p\n",
               infoPtr->hwndSelf, infoPtr->hMonthCal, infoPtr->hwndNotify, GetDesktopWindow ());
    }

    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return 0;
}


static LRESULT
DATETIME_LButtonUp (DATETIME_INFO *infoPtr)
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

    /* Not a click on the dropdown box, enabled it */
    infoPtr->bDropdownEnabled = TRUE;

    return 0;
}


static LRESULT
DATETIME_Button_Command (DATETIME_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    if( HIWORD(wParam) == BN_CLICKED) {
        DWORD state = SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0);
        infoPtr->dateValid = (state == BST_CHECKED);
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
        DATETIME_SendDateTimeChangeNotify(infoPtr);
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
DATETIME_Notify (DATETIME_INFO *infoPtr, const NMHDR *lpnmh)
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
        DATETIME_SendSimpleNotify(infoPtr, DTN_CLOSEUP);
    }
    if ((lpnmh->hwndFrom == infoPtr->hUpdown) && (lpnmh->code == UDN_DELTAPOS)) {
        const NM_UPDOWN *lpnmud = (const NM_UPDOWN*)lpnmh;
        TRACE("Delta pos %d\n", lpnmud->iDelta);
        infoPtr->pendingUpdown = lpnmud->iDelta;
    }
    return 0;
}


static LRESULT
DATETIME_KeyDown (DATETIME_INFO *infoPtr, DWORD vkCode)
{
    int fieldNum = infoPtr->select & DTHT_DATEFIELD;
    int wrap = 0;
    int new;

    if (!(infoPtr->haveFocus)) return 0;
    if ((fieldNum==0) && (infoPtr->select)) return 0;

    if (infoPtr->select & FORMATCALLMASK) {
	FIXME ("Callbacks not implemented yet\n");
    }

    switch (vkCode) {
	case VK_ADD:
    	case VK_UP:
	    infoPtr->nCharsEntered = 0;
	    DATETIME_IncreaseField (infoPtr, fieldNum, 1);
	    DATETIME_SendDateTimeChangeNotify (infoPtr);
	    break;
	case VK_SUBTRACT:
	case VK_DOWN:
	    infoPtr->nCharsEntered = 0;
	    DATETIME_IncreaseField (infoPtr, fieldNum, -1);
	    DATETIME_SendDateTimeChangeNotify (infoPtr);
	    break;
	case VK_HOME:
	    infoPtr->nCharsEntered = 0;
	    DATETIME_IncreaseField (infoPtr, fieldNum, INT_MIN);
	    DATETIME_SendDateTimeChangeNotify (infoPtr);
	    break;
	case VK_END:
	    infoPtr->nCharsEntered = 0;
	    DATETIME_IncreaseField (infoPtr, fieldNum, INT_MAX);
	    DATETIME_SendDateTimeChangeNotify (infoPtr);
	    break;
	case VK_LEFT:
	    new = infoPtr->select;
	    do {
		if (new == 0) {
		    new = new - 1;
		    wrap++;
		} else {
		    new--;
		}
	    } while ((infoPtr->fieldspec[new] & DT_STRING) && (wrap<2));
	    if (new != infoPtr->select)
	        DATETIME_SetSelectedField(infoPtr, new);
	    break;
	case VK_RIGHT:
	    new = infoPtr->select;
	    do {
		new++;
		if (new==infoPtr->nrFields) {
		    new = 0;
		    wrap++;
		}
	    } while ((infoPtr->fieldspec[new] & DT_STRING) && (wrap<2));
	    if (new != infoPtr->select)
	        DATETIME_SetSelectedField(infoPtr, new);
	    break;
    }

    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return 0;
}


static LRESULT
DATETIME_Char (DATETIME_INFO *infoPtr, WPARAM vkCode)
{
    int fieldNum, fieldSpec;

    fieldNum = infoPtr->select & DTHT_DATEFIELD;
    fieldSpec = infoPtr->fieldspec[fieldNum];

    if (fieldSpec == ONELETTERAMPM || fieldSpec == TWOLETTERAMPM) {
        infoPtr->charsEntered[0] = vkCode;
        infoPtr->nCharsEntered = 1;

        DATETIME_ApplySelectedField(infoPtr);
    } else if (vkCode >= '0' && vkCode <= '9') {
        int maxChars;

        infoPtr->charsEntered[infoPtr->nCharsEntered++] = vkCode;

        if (fieldSpec == INVALIDFULLYEAR || fieldSpec == FULLYEAR)
            maxChars = 4;
        else
            maxChars = 2;

        if ((fieldSpec == ONEDIGIT12HOUR ||
             fieldSpec == TWODIGIT12HOUR ||
             fieldSpec == ONEDIGIT24HOUR ||
             fieldSpec == TWODIGIT24HOUR) &&
            (infoPtr->nCharsEntered == 1))
        {
            if (vkCode >= '3')
                 maxChars = 1;
        }

        if (maxChars == infoPtr->nCharsEntered)
            DATETIME_ApplySelectedField(infoPtr);
    }

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
        DATETIME_SetSelectedField (infoPtr, -1);
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

    return 1;
}

static LRESULT DATETIME_NCPaint (HWND hwnd, HRGN region)
{
    INT cxEdge, cyEdge;
    HRGN clipRgn;
    HTHEME theme;
    LONG exStyle;
    RECT r;
    HDC dc;

    theme = OpenThemeDataForDpi(NULL, WC_EDITW, GetDpiForWindow(hwnd));
    if (!theme)
        return DefWindowProcW(hwnd, WM_NCPAINT, (WPARAM)region, 0);

    exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    if (!(exStyle & WS_EX_CLIENTEDGE))
    {
        CloseThemeData(theme);
        return DefWindowProcW(hwnd, WM_NCPAINT, (WPARAM)region, 0);
    }

    cxEdge = GetSystemMetrics(SM_CXEDGE);
    cyEdge = GetSystemMetrics(SM_CYEDGE);
    GetWindowRect(hwnd, &r);

    /* New clipping region passed to default proc to exclude border */
    clipRgn = CreateRectRgn(r.left + cxEdge, r.top + cyEdge, r.right - cxEdge, r.bottom - cyEdge);
    if (region != (HRGN)1)
        CombineRgn(clipRgn, clipRgn, region, RGN_AND);
    OffsetRect(&r, -r.left, -r.top);

    dc = GetDCEx(hwnd, region, DCX_WINDOW | DCX_INTERSECTRGN);
    if (IsThemeBackgroundPartiallyTransparent(theme, 0, 0))
        DrawThemeParentBackground(hwnd, dc, &r);
    DrawThemeBackground(theme, dc, 0, 0, &r, 0);
    ReleaseDC(hwnd, dc);
    CloseThemeData(theme);

    /* Call default proc to get the scrollbars etc. also painted */
    DefWindowProcW(hwnd, WM_NCPAINT, (WPARAM)clipRgn, 0);
    DeleteObject(clipRgn);
    return 0;
}

static LRESULT DATETIME_MouseMove (DATETIME_INFO *infoPtr, LONG x, LONG y)
{
    TRACKMOUSEEVENT event;
    POINT point;
    BOOL hot;

    point.x = x;
    point.y = y;
    hot = PtInRect(&infoPtr->calbutton, point);
    if (hot != infoPtr->bCalHot)
    {
        infoPtr->bCalHot = hot;
        RedrawWindow(infoPtr->hwndSelf, &infoPtr->calbutton, 0, RDW_INVALIDATE | RDW_UPDATENOW);
    }

    if (!hot)
        return 0;

    event.cbSize = sizeof(TRACKMOUSEEVENT);
    event.dwFlags = TME_QUERY;
    if (!TrackMouseEvent(&event) || event.hwndTrack != infoPtr->hwndSelf || !(event.dwFlags & TME_LEAVE))
    {
        event.hwndTrack = infoPtr->hwndSelf;
        event.dwFlags = TME_LEAVE;
        TrackMouseEvent(&event);
    }

    return 0;
}

static LRESULT DATETIME_MouseLeave (DATETIME_INFO *infoPtr)
{
    infoPtr->bCalHot = FALSE;
    RedrawWindow(infoPtr->hwndSelf, &infoPtr->calbutton, 0, RDW_INVALIDATE | RDW_UPDATENOW);
    return 0;
}

static LRESULT
DATETIME_SetFocus (DATETIME_INFO *infoPtr, HWND lostFocus)
{
    TRACE("got focus from %p\n", lostFocus);

    /* if monthcal is open and it loses focus, close monthcal */
    if (infoPtr->hMonthCal && (lostFocus == infoPtr->hMonthCal) &&
        IsWindowVisible(infoPtr->hMonthCal))
    {
        ShowWindow(infoPtr->hMonthCal, SW_HIDE);
        DATETIME_SendSimpleNotify(infoPtr, DTN_CLOSEUP);
        /* note: this get triggered even if monthcal loses focus to a dropdown
         * box click, which occurs without an intermediate WM_PAINT call
         */
        infoPtr->bDropdownEnabled = FALSE;
        return 0;
    }

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

    dtdtc.dwFlags = infoPtr->dateValid ? GDT_VALID : GDT_NONE;

    dtdtc.st = infoPtr->date;
    return (BOOL) SendMessageW (infoPtr->hwndNotify, WM_NOTIFY,
                                dtdtc.nmhdr.idFrom, (LPARAM)&dtdtc);
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
                                nmhdr.idFrom, (LPARAM)&nmhdr);
}

static LRESULT
DATETIME_Size (DATETIME_INFO *infoPtr, INT width, INT height)
{
    /* set size */
    infoPtr->rcClient.bottom = height;
    infoPtr->rcClient.right = width;

    TRACE("Height %ld, Width %ld\n", infoPtr->rcClient.bottom, infoPtr->rcClient.right);

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
DATETIME_StyleChanging(DATETIME_INFO *infoPtr, WPARAM wStyleType, STYLESTRUCT *lpss)
{
    TRACE("styletype %Ix, styleOld %#lx, styleNew %#lx\n", wStyleType, lpss->styleOld, lpss->styleNew);

    /* block DTS_SHOWNONE change */
    if ((lpss->styleNew ^ lpss->styleOld) & DTS_SHOWNONE)
    {
        if (lpss->styleOld & DTS_SHOWNONE)
            lpss->styleNew |= DTS_SHOWNONE;
        else
            lpss->styleNew &= ~DTS_SHOWNONE;
    }

    return 0;
}

static LRESULT 
DATETIME_StyleChanged(DATETIME_INFO *infoPtr, WPARAM wStyleType, const STYLESTRUCT *lpss)
{
    TRACE("styletype %Ix, styleOld %#lx, styleNew %#lx\n", wStyleType, lpss->styleOld, lpss->styleNew);

    if (wStyleType != GWL_STYLE) return 0;
  
    infoPtr->dwStyle = lpss->styleNew;

    if ( !(lpss->styleOld & DTS_SHOWNONE) && (lpss->styleNew & DTS_SHOWNONE) ) {
        infoPtr->hwndCheckbut = CreateWindowExW (0, WC_BUTTONW, 0, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
         					 2, 2, 13, 13, infoPtr->hwndSelf, 0, 
						(HINSTANCE)GetWindowLongPtrW (infoPtr->hwndSelf, GWLP_HINSTANCE), 0);
        SendMessageW (infoPtr->hwndCheckbut, BM_SETCHECK, infoPtr->dateValid ? 1 : 0, 0);
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
    return 0;
}

static LRESULT DATETIME_ThemeChanged (DATETIME_INFO *infoPtr)
{
    HTHEME theme;

    theme = GetWindowTheme(infoPtr->hwndSelf);
    CloseThemeData(theme);
    OpenThemeData(infoPtr->hwndSelf, themeClass);
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return 0;
}

static BOOL DATETIME_GetIdealSize(DATETIME_INFO *infoPtr, SIZE *size)
{
    SIZE field_size;
    RECT rect;
    WCHAR txt[80];
    HDC hdc;
    HFONT oldFont;
    int i;

    size->cx = size->cy = 0;

    hdc = GetDC(infoPtr->hwndSelf);
    oldFont = SelectObject(hdc, infoPtr->hFont);

    /* Get text font height */
    DATETIME_ReturnTxt(infoPtr, 0, txt, ARRAY_SIZE(txt));
    GetTextExtentPoint32W(hdc, txt, lstrlenW(txt), &field_size);
    size->cy = field_size.cy;

    /* Get text font width */
    for (i = 0; i < infoPtr->nrFields; i++)
    {
        size->cx += DATETIME_GetFieldWidth(infoPtr, hdc, i);
    }

    SelectObject(hdc, oldFont);
    ReleaseDC(infoPtr->hwndSelf, hdc);

    if (infoPtr->dwStyle & DTS_UPDOWN)
    {
        GetWindowRect(infoPtr->hUpdown, &rect);
        size->cx += rect.right - rect.left;
    }
    else
    {
        size->cx += infoPtr->calbutton.right - infoPtr->calbutton.left;
    }

    if (infoPtr->dwStyle & DTS_SHOWNONE)
    {
        size->cx += infoPtr->checkbox.right - infoPtr->checkbox.left;
    }

    /* Add space between controls for them not to get too close */
    size->cx += 12;
    size->cy += 4;

    TRACE("cx %ld, cy %ld\n", size->cx, size->cy);
    return TRUE;
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
    DATETIME_INFO *infoPtr = Alloc (sizeof(DATETIME_INFO));
    STYLESTRUCT ss = { 0, lpcs->style };

    if (!infoPtr) return -1;

    infoPtr->hwndSelf = hwnd;
    infoPtr->dwStyle = lpcs->style;

    infoPtr->nrFieldsAllocated = 32;
    infoPtr->fieldspec = Alloc (infoPtr->nrFieldsAllocated * sizeof(int));
    infoPtr->fieldRect = Alloc (infoPtr->nrFieldsAllocated * sizeof(RECT));
    infoPtr->buflen = Alloc (infoPtr->nrFieldsAllocated * sizeof(int));
    infoPtr->hwndNotify = lpcs->hwndParent;
    infoPtr->select = -1; /* initially, nothing is selected */
    infoPtr->bDropdownEnabled = TRUE;
    infoPtr->bCalHot = FALSE;

    DATETIME_StyleChanged(infoPtr, GWL_STYLE, &ss);
    DATETIME_SetFormatW (infoPtr, 0);

    /* create the monthcal control */
    infoPtr->hMonthCal = CreateWindowExW (0, MONTHCAL_CLASSW, 0, WS_BORDER | WS_POPUP | WS_CLIPSIBLINGS,
					  0, 0, 0, 0, infoPtr->hwndSelf, 0, 0, 0);

    /* initialize info structure */
    GetLocalTime (&infoPtr->date);
    infoPtr->dateValid = TRUE;
    infoPtr->hFont = GetStockObject(DEFAULT_GUI_FONT);

    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);
    OpenThemeData(hwnd, themeClass);

    return 0;
}



static LRESULT
DATETIME_Destroy (DATETIME_INFO *infoPtr)
{
    HTHEME theme;

    theme = GetWindowTheme(infoPtr->hwndSelf);
    CloseThemeData(theme);

    if (infoPtr->hwndCheckbut)
	DestroyWindow(infoPtr->hwndCheckbut);
    if (infoPtr->hUpdown)
	DestroyWindow(infoPtr->hUpdown);
    if (infoPtr->hMonthCal) 
        DestroyWindow(infoPtr->hMonthCal);
    SetWindowLongPtrW( infoPtr->hwndSelf, 0, 0 ); /* clear infoPtr */
    Free (infoPtr->buflen);
    Free (infoPtr->fieldRect);
    Free (infoPtr->fieldspec);
    Free (infoPtr);
    return 0;
}


static INT
DATETIME_GetText (const DATETIME_INFO *infoPtr, INT count, LPWSTR dst)
{
    WCHAR buf[80];
    int i;

    if (!dst || (count <= 0)) return 0;

    dst[0] = 0;
    for (i = 0; i < infoPtr->nrFields; i++)
    {
        DATETIME_ReturnTxt(infoPtr, i, buf, ARRAY_SIZE(buf));
        if ((lstrlenW(dst) + lstrlenW(buf)) < count)
            lstrcatW(dst, buf);
        else break;
    }
    return lstrlenW(dst);
}

static int DATETIME_GetTextLength(const DATETIME_INFO *info)
{
    int i, length = 0;
    WCHAR buffer[80];

    TRACE("%p.\n", info);

    for (i = 0; i < info->nrFields; i++)
    {
        DATETIME_ReturnTxt(info, i, buffer, ARRAY_SIZE(buffer));
        length += lstrlenW(buffer);
    }
    return length;
}

static LRESULT WINAPI
DATETIME_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr = ((DATETIME_INFO *)GetWindowLongPtrW (hwnd, 0));

    TRACE("%x, %Ix, %Ix\n", uMsg, wParam, lParam);

    if (!infoPtr && (uMsg != WM_CREATE) && (uMsg != WM_NCCREATE))
	return DefWindowProcW( hwnd, uMsg, wParam, lParam );

    switch (uMsg) {

    case DTM_GETSYSTEMTIME:
        return DATETIME_GetSystemTime (infoPtr, (SYSTEMTIME *) lParam);

    case DTM_SETSYSTEMTIME:
	return DATETIME_SetSystemTime (infoPtr, wParam, (SYSTEMTIME *) lParam);

    case DTM_GETRANGE:
	return SendMessageW (infoPtr->hMonthCal, MCM_GETRANGE, wParam, lParam);

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

    case DTM_GETIDEALSIZE:
        return DATETIME_GetIdealSize(infoPtr, (SIZE *)lParam);

    case WM_NOTIFY:
	return DATETIME_Notify (infoPtr, (LPNMHDR)lParam);

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
        return DATETIME_KeyDown (infoPtr, wParam);

    case WM_CHAR:
        return DATETIME_Char (infoPtr, wParam);

    case WM_KILLFOCUS:
        return DATETIME_KillFocus (infoPtr, (HWND)wParam);

    case WM_NCCREATE:
        return DATETIME_NCCreate (hwnd, (LPCREATESTRUCTW)lParam);

    case WM_NCPAINT:
        return DATETIME_NCPaint(hwnd, (HRGN)wParam);

    case WM_MOUSEMOVE:
        return DATETIME_MouseMove(infoPtr, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

    case WM_MOUSELEAVE:
        return DATETIME_MouseLeave(infoPtr);

    case WM_SETFOCUS:
        return DATETIME_SetFocus (infoPtr, (HWND)wParam);

    case WM_SIZE:
        return DATETIME_Size (infoPtr, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

    case WM_LBUTTONDOWN:
        return DATETIME_LButtonDown (infoPtr, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

    case WM_LBUTTONUP:
        return DATETIME_LButtonUp (infoPtr);

    case WM_VSCROLL:
        return DATETIME_VScroll (infoPtr, (WORD)wParam);

    case WM_CREATE:
	return DATETIME_Create (hwnd, (LPCREATESTRUCTW)lParam);

    case WM_DESTROY:
	return DATETIME_Destroy (infoPtr);

    case WM_COMMAND:
        return DATETIME_Command (infoPtr, wParam, lParam);

    case WM_STYLECHANGING:
        return DATETIME_StyleChanging(infoPtr, wParam, (LPSTYLESTRUCT)lParam);

    case WM_STYLECHANGED:
        return DATETIME_StyleChanged(infoPtr, wParam, (LPSTYLESTRUCT)lParam);

    case WM_THEMECHANGED:
        return DATETIME_ThemeChanged(infoPtr);

    case WM_SETFONT:
        return DATETIME_SetFont(infoPtr, (HFONT)wParam, (BOOL)lParam);

    case WM_GETFONT:
        return (LRESULT) infoPtr->hFont;

    case WM_GETTEXT:
        return (LRESULT) DATETIME_GetText(infoPtr, wParam, (LPWSTR)lParam);

    case WM_GETTEXTLENGTH:
        return (LRESULT)DATETIME_GetTextLength(infoPtr);

    case WM_SETTEXT:
        return CB_ERR;

    default:
        if ((uMsg >= WM_USER) && (uMsg < WM_APP) && !COMCTL32_IsReflectedMessage(uMsg))
            ERR("unknown msg %04x, wp %Ix, lp %Ix\n", uMsg, wParam, lParam);
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
