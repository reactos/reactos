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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * TODO:
 *   - All messages.
 *   - All notifications.
 *
 */

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(datetime);

typedef struct
{
	HWND hMonthCal;
	HWND hwndNotify;
	HWND hUpdown;
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
	char textbuf[256];
        POINT monthcal_pos;
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

#define DATETIME_GetInfoPtr(hwnd) ((DATETIME_INFO *)GetWindowLongA (hwnd, 0))

static BOOL DATETIME_SendSimpleNotify (HWND hwnd, UINT code);
static BOOL DATETIME_SendDateTimeChangeNotify (HWND hwnd);
extern void MONTHCAL_CopyTime(const SYSTEMTIME *from, SYSTEMTIME *to);
static const char *allowedformatchars = {"dhHmMstyX'"};
static const int maxrepetition [] = {4,2,2,2,4,2,2,4,-1,-1};


static LRESULT
DATETIME_GetSystemTime (HWND hwnd, WPARAM wParam, LPARAM lParam )
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);
  DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
  SYSTEMTIME *lprgSysTimeArray=(SYSTEMTIME *) lParam;

  TRACE("%04x %08lx\n",wParam,lParam);
  if (!lParam) return GDT_NONE;

  if ((dwStyle & DTS_SHOWNONE) &&
       (SendMessageA (infoPtr->hwndCheckbut, BM_GETCHECK, 0, 0) == BST_UNCHECKED))
        return GDT_NONE;

  MONTHCAL_CopyTime (&infoPtr->date, lprgSysTimeArray);

  return GDT_VALID;
}


static LRESULT
DATETIME_SetSystemTime (HWND hwnd, WPARAM wParam, LPARAM lParam )
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);
  SYSTEMTIME *lprgSysTimeArray=(SYSTEMTIME *) lParam;

  TRACE("%p %04x %08lx\n",hwnd, wParam, lParam);
  if (!lParam) return 0;

  TRACE("%04d/%02d/%02d %02d:%02d:%02d\n",
        lprgSysTimeArray->wYear, lprgSysTimeArray->wMonth, lprgSysTimeArray->wDay,
        lprgSysTimeArray->wHour, lprgSysTimeArray->wMinute, lprgSysTimeArray->wSecond);

  if (wParam==GDT_VALID) {
      infoPtr->dateValid = TRUE;
      MONTHCAL_CopyTime (lprgSysTimeArray, &infoPtr->date);
	SendMessageA(infoPtr->hMonthCal, MCM_SETCURSEL, 0, (LPARAM)(&infoPtr->date));
      SendMessageA (infoPtr->hwndCheckbut, BM_SETCHECK, BST_CHECKED, 0);
  } else if (wParam==GDT_NONE) {
      infoPtr->dateValid = FALSE;
      SendMessageA (infoPtr->hwndCheckbut, BM_SETCHECK, BST_UNCHECKED, 0);
  }
  InvalidateRect(hwnd, NULL, TRUE);
  return 1;
}


static LRESULT
DATETIME_GetRange (HWND hwnd, LPARAM lParam )
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);
  LRESULT ret;

  TRACE("%08lx\n", lParam);
  ret =  SendMessageA (infoPtr->hMonthCal, MCM_GETRANGE, 0, lParam);
  if (!ret) ret = 1; /* bug emulation... */
  return ret;
}

static LRESULT
DATETIME_SetRange (HWND hwnd, WPARAM wParam, LPARAM lParam )
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);

  TRACE("%04x %08lx\n",wParam,lParam);

  return SendMessageA (infoPtr->hMonthCal, MCM_SETRANGE, wParam, lParam);
}

static LRESULT
DATETIME_GetMonthCalColor (HWND hwnd, WPARAM wParam)
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);

  TRACE("%04x\n",wParam);
  return SendMessageA (infoPtr->hMonthCal, MCM_GETCOLOR, wParam, 0);
}


static LRESULT
DATETIME_SetMonthCalColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);

  TRACE("%04x %08lx\n",wParam,lParam);
  return SendMessageA (infoPtr->hMonthCal, MCM_SETCOLOR, wParam, lParam);
}


/* FIXME: need to get way to force font into monthcal structure */
static LRESULT
DATETIME_GetMonthCal (HWND hwnd)
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);

  TRACE("\n");
  return (LRESULT)infoPtr->hMonthCal;
}



/* FIXME: need to get way to force font into monthcal structure */

static LRESULT
DATETIME_GetMonthCalFont (HWND hwnd)
{

  TRACE("\n");
  return 0;
}


static LRESULT
DATETIME_SetMonthCalFont (HWND hwnd, WPARAM wParam, LPARAM lParam)
{

  TRACE("%04x %08lx\n",wParam,lParam);
  return 0;
}


/*
   Split up a formattxt in actions.
   See ms documentation for the meaning of the letter codes/'specifiers'.

   Notes:
   *'dddddd' is handled as 'dddd' plus 'dd'.
   *unrecognized formats are strings (here given the type DT_STRING;
   start of the string is encoded in lower bits of DT_STRING.
   Therefore, 'string' ends finally up as '<show seconds>tring'.

 */


static void
DATETIME_UseFormat (DATETIME_INFO *infoPtr, const char *formattxt)
{
 int i,j,k,len;
 int *nrFields=& infoPtr->nrFields;

 TRACE ("%s\n",formattxt);


 *nrFields=0;
 infoPtr->fieldspec[*nrFields]=0;
 len=strlen(allowedformatchars);
 k=0;

 for (i=0; i<strlen (formattxt); i++)  {
	TRACE ("\n%d %c:",i, formattxt[i]);
 	for (j=0; j<len; j++) {
 		if (allowedformatchars[j]==formattxt[i]) {
			TRACE ("%c[%d,%x]",allowedformatchars[j], *nrFields,
							 infoPtr->fieldspec[*nrFields]);
			if ((*nrFields==0) && (infoPtr->fieldspec[*nrFields]==0)) {
				infoPtr->fieldspec[*nrFields]=(j<<4) +1;
				break;
			}
			if (infoPtr->fieldspec[*nrFields]>>4!=j) {
				(*nrFields)++;
				infoPtr->fieldspec[*nrFields]=(j<<4) +1;
				break;
			}
			if ((infoPtr->fieldspec[*nrFields] & 0x0f)==maxrepetition[j]) {
				(*nrFields)++;
				infoPtr->fieldspec[*nrFields]=(j<<4) +1;
				break;
			}
			infoPtr->fieldspec[*nrFields]++;
			break;
		}   /* if allowedformatchar */
	 } /* for j */


			/* char is not a specifier: handle char like a string */
	if (j==len) {
		if ((*nrFields==0) && (infoPtr->fieldspec[*nrFields]==0)) {
			infoPtr->fieldspec[*nrFields]=DT_STRING+k;
			infoPtr->buflen[*nrFields]=0;
        } else
		if ((infoPtr->fieldspec[*nrFields] & DT_STRING)!=DT_STRING)  {
			(*nrFields)++;
			infoPtr->fieldspec[*nrFields]=DT_STRING+k;
			infoPtr->buflen[*nrFields]=0;
		}
		infoPtr->textbuf[k]=formattxt[i];
		k++;
		infoPtr->buflen[*nrFields]++;
	}   /* if j=len */

	if (*nrFields==infoPtr->nrFieldsAllocated) {
		FIXME ("out of memory; should reallocate. crash ahead.\n");
	}

  } /* for i */

  TRACE("\n");

  if (infoPtr->fieldspec[*nrFields]!=0) (*nrFields)++;
}


static LRESULT
DATETIME_SetFormat (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
 DATETIME_INFO *infoPtr= DATETIME_GetInfoPtr (hwnd);
 char format_buf[80];
 DWORD format_item;

 TRACE("%04x %08lx\n",wParam,lParam);
 if (!lParam) {
  	DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);

	if (dwStyle & DTS_LONGDATEFORMAT)
		format_item=LOCALE_SLONGDATE;
	else if (dwStyle & DTS_TIMEFORMAT)
		format_item=LOCALE_STIMEFORMAT;
        else /* DTS_SHORTDATEFORMAT */
		format_item=LOCALE_SSHORTDATE;
	GetLocaleInfoA( GetSystemDefaultLCID(), format_item,format_buf,sizeof(format_buf));
	DATETIME_UseFormat (infoPtr, format_buf);
 }
 else
 	DATETIME_UseFormat (infoPtr, (char *) lParam);

 return infoPtr->nrFields;
}


static LRESULT
DATETIME_SetFormatW (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
 TRACE("%04x %08lx\n",wParam,lParam);
 if (lParam) {
	LPSTR buf;
	int retval;
        int len = WideCharToMultiByte( CP_ACP, 0, (LPWSTR)lParam, -1, NULL, 0, NULL, NULL );

 	buf = (LPSTR) Alloc (len);
        WideCharToMultiByte( CP_ACP, 0, (LPWSTR)lParam, -1, buf, len, NULL, NULL );
	retval=DATETIME_SetFormat (hwnd, 0, (LPARAM) buf);
	Free (buf);
	return retval;
 }
 else
	return DATETIME_SetFormat (hwnd, 0, 0);

}


static void
DATETIME_ReturnTxt (DATETIME_INFO *infoPtr, int count, char *result, int resultSize)
{
 SYSTEMTIME date = infoPtr->date;
 int spec;
 char buffer[80];

 *result=0;
 TRACE ("%d,%d\n", infoPtr->nrFields, count);
 if ((count>infoPtr->nrFields) || (count<0)) {
	WARN ("buffer overrun, have %d want %d\n", infoPtr->nrFields, count);
	return;
 }

 if (!infoPtr->fieldspec) return;

 spec=infoPtr->fieldspec[count];
 if (spec & DT_STRING) {
	int txtlen=infoPtr->buflen[count];

        if (txtlen > resultSize)
            txtlen = resultSize - 1;
	memcpy (result, infoPtr->textbuf + (spec &~ DT_STRING), txtlen);
	result[txtlen]=0;
	TRACE ("arg%d=%x->[%s]\n",count,infoPtr->fieldspec[count],result);
	return;
 }


 switch (spec) {
	case DT_END_FORMAT:
		*result=0;
		break;
	case ONEDIGITDAY:
		sprintf (result,"%d",date.wDay);
	 	break;
	case TWODIGITDAY:
		sprintf (result,"%.2d",date.wDay);
		break;
	case THREECHARDAY:
	        GetLocaleInfoA( LOCALE_USER_DEFAULT, LOCALE_SABBREVDAYNAME1+(date.wDayOfWeek+6)%7,
				result,4);
		/*sprintf (result,"%.3s",days[date.wDayOfWeek]);*/
		break;
	case FULLDAY:
	        GetLocaleInfoA( LOCALE_USER_DEFAULT,LOCALE_SDAYNAME1+ (date.wDayOfWeek+6)%7,
                               result, resultSize);
		break;
	case ONEDIGIT12HOUR:
		if (date.wHour>12)
			sprintf (result,"%d",date.wHour-12);
		else
			sprintf (result,"%d",date.wHour);
		break;
	case TWODIGIT12HOUR:
		if (date.wHour>12)
			sprintf (result,"%.2d",date.wHour-12);
		else
			sprintf (result,"%.2d",date.wHour);
		break;
	case ONEDIGIT24HOUR:
		sprintf (result,"%d",date.wHour);
		break;
	case TWODIGIT24HOUR:
		sprintf (result,"%.2d",date.wHour);
		break;
	case ONEDIGITSECOND:
		sprintf (result,"%d",date.wSecond);
		break;
	case TWODIGITSECOND:
		sprintf (result,"%.2d",date.wSecond);
		break;
	case ONEDIGITMINUTE:
		sprintf (result,"%d",date.wMinute);
		break;
	case TWODIGITMINUTE:
		sprintf (result,"%.2d",date.wMinute);
		break;
	case ONEDIGITMONTH:
		sprintf (result,"%d",date.wMonth);
	 	break;
	case TWODIGITMONTH:
		sprintf (result,"%.2d",date.wMonth);
		break;
	case THREECHARMONTH:
		GetLocaleInfoA( GetSystemDefaultLCID(),LOCALE_SMONTHNAME1+date.wMonth -1,
                               buffer,sizeof(buffer));
		sprintf (result,"%.3s",buffer);
		break;
	case FULLMONTH:
		GetLocaleInfoA( GetSystemDefaultLCID(),LOCALE_SMONTHNAME1+date.wMonth -1,
                               result, resultSize);
		break;
	case ONELETTERAMPM:
		if (date.wHour<12)
			strcpy (result,"A");
		else
			strcpy (result,"P");
		break;
	case TWOLETTERAMPM:
		if (date.wHour<12)
			strcpy (result,"AM");
		else
			strcpy (result,"PM");
		break;
	case FORMATCALLBACK:
		FIXME ("Not implemented\n");
		strcpy (result,"xxx");
		break;
	case ONEDIGITYEAR:
		sprintf (result,"%d",date.wYear-10* (int) floor(date.wYear/10));
	 	break;
	case TWODIGITYEAR:
		sprintf (result,"%.2d",date.wYear-100* (int) floor(date.wYear/100));
		break;
        case INVALIDFULLYEAR:
	case FULLYEAR:
		sprintf (result,"%d",date.wYear);
		break;
    }

	TRACE ("arg%d=%x->[%s]\n",count,infoPtr->fieldspec[count],result);
}


static void
DATETIME_IncreaseField (DATETIME_INFO *infoPtr, int number)
{
 SYSTEMTIME *date = &infoPtr->date;
 int spec;

 TRACE ("%d\n",number);
 if ((number>infoPtr->nrFields) || (number<0)) return;

 spec=infoPtr->fieldspec[number];
 if ((spec & DTHT_DATEFIELD)==0) return;

 switch (spec) {
	case ONEDIGITDAY:
	case TWODIGITDAY:
	case THREECHARDAY:
	case FULLDAY:
		date->wDay++;
		if (date->wDay>MONTHCAL_MonthLength(date->wMonth,date->wYear))
		  date->wDay=1;
		break;
	case ONEDIGIT12HOUR:
	case TWODIGIT12HOUR:
	case ONEDIGIT24HOUR:
	case TWODIGIT24HOUR:
		date->wHour++;
		if (date->wHour>23) date->wHour=0;
		break;
	case ONEDIGITSECOND:
	case TWODIGITSECOND:
		date->wSecond++;
		if (date->wSecond>59) date->wSecond=0;
		break;
	case ONEDIGITMINUTE:
	case TWODIGITMINUTE:
		date->wMinute++;
		if (date->wMinute>59) date->wMinute=0;
		break;
	case ONEDIGITMONTH:
	case TWODIGITMONTH:
	case THREECHARMONTH:
	case FULLMONTH:
		date->wMonth++;
		if (date->wMonth>12) date->wMonth=1;
		if (date->wDay>MONTHCAL_MonthLength(date->wMonth,date->wYear))
			date->wDay=MONTHCAL_MonthLength(date->wMonth,date->wYear);
		break;
	case ONELETTERAMPM:
	case TWOLETTERAMPM:
		date->wHour+=12;
		if (date->wHour>23) date->wHour-=24;
		break;
	case FORMATCALLBACK:
		FIXME ("Not implemented\n");
		break;
	case ONEDIGITYEAR:
	case TWODIGITYEAR:
	case FULLYEAR:
		date->wYear++;
		break;
	}

}


static void
DATETIME_DecreaseField (DATETIME_INFO *infoPtr, int number)
{
 SYSTEMTIME *date = & infoPtr->date;
 int spec;

 TRACE ("%d\n",number);
 if ((number>infoPtr->nrFields) || (number<0)) return;

 spec = infoPtr->fieldspec[number];
 if ((spec & DTHT_DATEFIELD)==0) return;

 TRACE ("%x\n",spec);

 switch (spec) {
	case ONEDIGITDAY:
	case TWODIGITDAY:
	case THREECHARDAY:
	case FULLDAY:
		date->wDay--;
		if (date->wDay<1)
		  date->wDay=MONTHCAL_MonthLength(date->wMonth,date->wYear);
		break;
	case ONEDIGIT12HOUR:
	case TWODIGIT12HOUR:
	case ONEDIGIT24HOUR:
	case TWODIGIT24HOUR:
		if (date->wHour)
			date->wHour--;
		else
			date->wHour=23;
		break;
	case ONEDIGITSECOND:
	case TWODIGITSECOND:
		if (date->wHour)
			date->wSecond--;
		else
			date->wHour=59;
		break;
	case ONEDIGITMINUTE:
	case TWODIGITMINUTE:
		if (date->wMinute)
			date->wMinute--;
		else
			date->wMinute=59;
		break;
	case ONEDIGITMONTH:
	case TWODIGITMONTH:
	case THREECHARMONTH:
	case FULLMONTH:
		if (date->wMonth>1)
			date->wMonth--;
		else
			date->wMonth=12;
		if (date->wDay>MONTHCAL_MonthLength(date->wMonth,date->wYear))
			date->wDay=MONTHCAL_MonthLength(date->wMonth,date->wYear);
		break;
	case ONELETTERAMPM:
	case TWOLETTERAMPM:
		if (date->wHour<12)
			date->wHour+=12;
		else
			date->wHour-=12;
		break;
	case FORMATCALLBACK:
		FIXME ("Not implemented\n");
		break;
	case ONEDIGITYEAR:
	case TWODIGITYEAR:
	case FULLYEAR:
		date->wYear--;
		break;
	}

}


static void
DATETIME_ResetFieldDown (DATETIME_INFO *infoPtr, int number)
{
 SYSTEMTIME *date = &infoPtr->date;
 int spec;

 TRACE ("%d\n",number);
 if ((number>infoPtr->nrFields) || (number<0)) return;

 spec = infoPtr->fieldspec[number];
 if ((spec & DTHT_DATEFIELD)==0) return;


 switch (spec) {
	case ONEDIGITDAY:
	case TWODIGITDAY:
	case THREECHARDAY:
	case FULLDAY:
		date->wDay = 1;
		break;
	case ONEDIGIT12HOUR:
	case TWODIGIT12HOUR:
	case ONEDIGIT24HOUR:
	case TWODIGIT24HOUR:
	case ONELETTERAMPM:
	case TWOLETTERAMPM:
		date->wHour = 0;
		break;
	case ONEDIGITSECOND:
	case TWODIGITSECOND:
		date->wSecond = 0;
		break;
	case ONEDIGITMINUTE:
	case TWODIGITMINUTE:
		date->wMinute = 0;
		break;
	case ONEDIGITMONTH:
	case TWODIGITMONTH:
	case THREECHARMONTH:
	case FULLMONTH:
		date->wMonth = 1;
	case FORMATCALLBACK:
		FIXME ("Not implemented\n");
		break;
	case ONEDIGITYEAR:
	case TWODIGITYEAR:
        /* FYI: On 1752/9/14 the calendar changed and England and the
         * American colonies changed to the Gregorian calendar. This change
         * involved having September 14th follow September 2nd. So no date
         * algorithm works before that date.
         */
	case FULLYEAR:
		date->wSecond = 0;
		date->wMinute = 0;
		date->wHour = 0;
		date->wDay = 14;		/* overactive ms-programmers..*/
		date->wMonth = 9;
		date->wYear = 1752;
		break;
	}

}


static void
DATETIME_ResetFieldUp (DATETIME_INFO *infoPtr, int number)
{
 SYSTEMTIME *date = & infoPtr->date;
 int spec;

 TRACE("%d \n",number);
 if ((number>infoPtr->nrFields) || (number<0)) return;

 spec=infoPtr->fieldspec[number];
 if ((spec & DTHT_DATEFIELD)==0) return;

 switch (spec) {
	case ONEDIGITDAY:
	case TWODIGITDAY:
	case THREECHARDAY:
	case FULLDAY:
		date->wDay=MONTHCAL_MonthLength(date->wMonth,date->wYear);
		break;
	case ONEDIGIT12HOUR:
	case TWODIGIT12HOUR:
	case ONEDIGIT24HOUR:
	case TWODIGIT24HOUR:
	case ONELETTERAMPM:
	case TWOLETTERAMPM:
		date->wHour=23;
		break;
	case ONEDIGITSECOND:
	case TWODIGITSECOND:
		date->wSecond=59;
		break;
	case ONEDIGITMINUTE:
	case TWODIGITMINUTE:
		date->wMinute=59;
		break;
	case ONEDIGITMONTH:
	case TWODIGITMONTH:
	case THREECHARMONTH:
	case FULLMONTH:
		date->wMonth=12;
	case FORMATCALLBACK:
		FIXME ("Not implemented\n");
		break;
	case ONEDIGITYEAR:
	case TWODIGITYEAR:
	case FULLYEAR:
		date->wYear=9999;    /* Y10K problem? naaah. */
		break;
	}

}


static void DATETIME_Refresh (HWND hwnd, HDC hdc)

{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);
  int i,prevright;
  RECT *field;
  DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
  RECT *rcDraw = &infoPtr->rcDraw;
  RECT *rcClient = &infoPtr->rcClient;
  RECT *calbutton = &infoPtr->calbutton;
  RECT *checkbox = &infoPtr->checkbox;
  HBRUSH hbr;
  SIZE size;
  COLORREF oldBk, oldTextColor;

  /* draw control edge */
  TRACE("\n");
  hbr = CreateSolidBrush(RGB(255, 255, 255));
  FillRect(hdc, rcClient, hbr);
  DrawEdge(hdc, rcClient, EDGE_SUNKEN, BF_RECT);
  DeleteObject(hbr);

  if (infoPtr->dateValid) {
    char txt[80];
    HFONT oldFont;
    oldFont = SelectObject (hdc, infoPtr->hFont);

    DATETIME_ReturnTxt (infoPtr, 0, txt, sizeof(txt));
    GetTextExtentPoint32A (hdc, txt, strlen (txt), &size);
    rcDraw->bottom = size.cy+2;

    if (dwStyle & DTS_SHOWNONE)
      checkbox->right = 18;
    else
      checkbox->right = 2;	 
    prevright = checkbox->right;

    for (i=0; i<infoPtr->nrFields; i++) {
      DATETIME_ReturnTxt (infoPtr, i, txt, sizeof(txt));
      GetTextExtentPoint32A (hdc, txt, strlen (txt), &size);
      field = & infoPtr->fieldRect[i];
      field->left  = prevright;
      field->right = prevright+size.cx;
      field->top   = rcDraw->top;
      field->bottom = rcDraw->bottom;
      prevright = field->right;

      if ((infoPtr->haveFocus) && (i==infoPtr->select)) {
        hbr = CreateSolidBrush (GetSysColor (COLOR_ACTIVECAPTION));
        FillRect(hdc, field, hbr);
        oldBk = SetBkColor (hdc, GetSysColor(COLOR_ACTIVECAPTION));
        oldTextColor = SetTextColor (hdc, GetSysColor(COLOR_WINDOW));
        DeleteObject (hbr);
        DrawTextA ( hdc, txt, strlen(txt), field,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE );
        SetBkColor (hdc, oldBk);
		SetTextColor (hdc, oldTextColor);
      }
      else
        DrawTextA ( hdc, txt, strlen(txt), field,
                         DT_RIGHT | DT_VCENTER | DT_SINGLELINE );
    }

    SelectObject (hdc, oldFont);
  }

  if (!(dwStyle & DTS_UPDOWN)) {
    DrawFrameControl(hdc, calbutton, DFC_SCROLL,
        DFCS_SCROLLDOWN | (infoPtr->bCalDepressed ? DFCS_PUSHED : 0) |
        (dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0) );
  }
}


static LRESULT
DATETIME_HitTest (HWND hwnd, DATETIME_INFO *infoPtr, POINT pt)
{
  int i, retval;

  TRACE ("%ld, %ld\n",pt.x,pt.y);

  retval = DTHT_NONE;
  if (PtInRect (&infoPtr->calbutton, pt))
    {retval = DTHT_MCPOPUP; TRACE("Hit in calbutton(DTHT_MCPOPUP)\n"); goto done; }
  if (PtInRect (&infoPtr->checkbox, pt))
    {retval = DTHT_CHECKBOX; TRACE("Hit in checkbox(DTHT_CHECKBOX)\n"); goto done; }

  for (i=0; i<infoPtr->nrFields; i++) {
    if (PtInRect (&infoPtr->fieldRect[i], pt)) {
      retval = i;
      TRACE("Hit in date text in field %d\n", i);
      break;
    }
 }

done:
  return retval;
}


static LRESULT
DATETIME_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);
  DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
  int old, new;
  POINT pt;

  TRACE ("\n");

  old = infoPtr->select;
  pt.x = (INT)LOWORD(lParam);
  pt.y = (INT)HIWORD(lParam);

  new = DATETIME_HitTest (hwnd, infoPtr, pt);

  /* FIXME: might be conditions where we don't want to update infoPtr->select */
  infoPtr->select = new;

  if (infoPtr->select != old) {
    infoPtr->haveFocus = DTHT_GOTFOCUS;
   }

  if (infoPtr->select == DTHT_MCPOPUP) {
    /* FIXME: button actually is only depressed during dropdown of the */
    /* calendar control and when the mouse is over the button window */
    infoPtr->bCalDepressed = TRUE;

    /* recalculate the position of the monthcal popup */
    if(dwStyle & DTS_RIGHTALIGN)
      infoPtr->monthcal_pos.x = infoPtr->rcClient.right - ((infoPtr->calbutton.right -
                                infoPtr->calbutton.left) + 200);
    else
      infoPtr->monthcal_pos.x = 8;

    infoPtr->monthcal_pos.y = infoPtr->rcClient.bottom;
    ClientToScreen (hwnd, &(infoPtr->monthcal_pos));
    /* FIXME My Windoze has cx=about 200, but it probably depends on font size etc */
    SetWindowPos(infoPtr->hMonthCal, 0, infoPtr->monthcal_pos.x,
        infoPtr->monthcal_pos.y, 200, 150, 0);

    if(IsWindowVisible(infoPtr->hMonthCal))
        ShowWindow(infoPtr->hMonthCal, SW_HIDE);
    else {
      SYSTEMTIME *lprgSysTimeArray = &infoPtr->date;
      TRACE("update calendar %04d/%02d/%02d\n", 
        lprgSysTimeArray->wYear, lprgSysTimeArray->wMonth, lprgSysTimeArray->wDay);
      SendMessageA(infoPtr->hMonthCal, MCM_SETCURSEL, 0, (LPARAM)(&infoPtr->date));
      ShowWindow(infoPtr->hMonthCal, SW_SHOW);
    }

    TRACE ("dt:%p mc:%p mc parent:%p, desktop:%p\n",
           hwnd, infoPtr->hMonthCal, infoPtr->hwndNotify, GetDesktopWindow ());
    DATETIME_SendSimpleNotify (hwnd, DTN_DROPDOWN);
  }

  InvalidateRect(hwnd, NULL, FALSE);

  return 0;
}


static LRESULT
DATETIME_LButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);

  TRACE("\n");

  if(infoPtr->bCalDepressed == TRUE) {
    infoPtr->bCalDepressed = FALSE;
    InvalidateRect(hwnd, &(infoPtr->calbutton), TRUE);
  }

  return 0;
}


static LRESULT
DATETIME_Paint (HWND hwnd, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = wParam==0 ? BeginPaint (hwnd, &ps) : (HDC)wParam;
    DATETIME_Refresh (hwnd, hdc);
    if(!wParam)
    EndPaint (hwnd, &ps);
    return 0;
}


static LRESULT
DATETIME_Button_Command (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr(hwnd);

    switch(HIWORD(wParam)) {
    case BN_CLICKED:
    {
        DWORD state = SendMessageA((HWND)lParam, BM_GETCHECK, 0, 0);
        if(state == BST_CHECKED)
            infoPtr->dateValid = TRUE;
        else
            infoPtr->dateValid = FALSE;
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }
    default:
        return 0;
    }
}
          
        
        
static LRESULT
DATETIME_Command (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr(hwnd);

    TRACE("%08x %08lx\n", wParam, lParam);
    TRACE("hwndbutton = %p\n", infoPtr->hwndCheckbut);
    if(infoPtr->hwndCheckbut == (HWND)lParam)
        return DATETIME_Button_Command(hwnd, wParam, lParam);
    return 0;
}

static LRESULT
DATETIME_ParentNotify (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
 DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);
 LPNMHDR lpnmh = (LPNMHDR) lParam;

 TRACE ("%x,%lx\n",wParam, lParam);
 TRACE ("Got notification %x from %p\n", lpnmh->code, lpnmh->hwndFrom);
 TRACE ("info: %p %p %p\n",hwnd,infoPtr->hMonthCal,infoPtr->hUpdown);
 return 0;
}


static LRESULT
DATETIME_Notify (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
 DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);
 LPNMHDR lpnmh = (LPNMHDR) lParam;

 TRACE ("%x,%lx\n",wParam, lParam);
 TRACE ("Got notification %x from %p\n", lpnmh->code, lpnmh->hwndFrom);
 TRACE ("info: %p %p %p\n",hwnd,infoPtr->hMonthCal,infoPtr->hUpdown);

 if (lpnmh->code==MCN_SELECT) {
   ShowWindow(infoPtr->hMonthCal, SW_HIDE);
   infoPtr->dateValid = TRUE;
   SendMessageA (infoPtr->hMonthCal, MCM_GETCURSEL, 0, (LPARAM)&infoPtr->date);
   TRACE("got from calendar %04d/%02d/%02d\n", 
      infoPtr->date.wYear, infoPtr->date.wMonth, infoPtr->date.wDay);
   SendMessageA (infoPtr->hwndCheckbut, BM_SETCHECK, BST_CHECKED, 0);
   InvalidateRect(hwnd, NULL, TRUE);
   DATETIME_SendDateTimeChangeNotify (hwnd);
 }
 return 0;
}


static LRESULT
DATETIME_KeyDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
 DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);
 int FieldNum,wrap=0;

 TRACE("%x %lx %x\n",wParam, lParam, infoPtr->select);

 FieldNum = infoPtr->select & DTHT_DATEFIELD;

 if (!(infoPtr->haveFocus)) return 0;
 if ((FieldNum==0) && (infoPtr->select)) return 0;

 if (infoPtr->select & FORMATCALLMASK) {
	FIXME ("Callbacks not implemented yet\n");
 }

 switch (wParam) {
	case VK_ADD:
    	case VK_UP:
		DATETIME_IncreaseField (infoPtr,FieldNum);
		DATETIME_SendDateTimeChangeNotify (hwnd);
		break;
	case VK_SUBTRACT:
	case VK_DOWN:
		DATETIME_DecreaseField (infoPtr,FieldNum);
		DATETIME_SendDateTimeChangeNotify (hwnd);
		break;
	case VK_HOME:
		DATETIME_ResetFieldDown (infoPtr,FieldNum);
		DATETIME_SendDateTimeChangeNotify (hwnd);
		break;
	case VK_END:
		DATETIME_ResetFieldUp(infoPtr,FieldNum);
		DATETIME_SendDateTimeChangeNotify (hwnd);
		break;
	case VK_LEFT:
		do {
			if (infoPtr->select==0) {
				infoPtr->select = infoPtr->nrFields - 1;
				wrap++;
			} else
			infoPtr->select--;
		}
		while ((infoPtr->fieldspec[infoPtr->select] & DT_STRING) && (wrap<2));
		break;
	case VK_RIGHT:
		do {
			infoPtr->select++;
			if (infoPtr->select==infoPtr->nrFields) {
				infoPtr->select = 0;
				wrap++;
			}
			}
		while ((infoPtr->fieldspec[infoPtr->select] & DT_STRING) && (wrap<2));
		break;
	}

  InvalidateRect(hwnd, NULL, FALSE);

  return 0;
}


static LRESULT
DATETIME_KillFocus (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);

    TRACE ("\n");

    if (infoPtr->haveFocus) {
	DATETIME_SendSimpleNotify (hwnd, NM_KILLFOCUS);
	infoPtr->haveFocus = 0;
    }

    InvalidateRect (hwnd, NULL, TRUE);

    return 0;
}


static LRESULT
DATETIME_SetFocus (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);

    TRACE ("\n");

    if (infoPtr->haveFocus==0) {
	DATETIME_SendSimpleNotify (hwnd, NM_SETFOCUS);
	infoPtr->haveFocus = DTHT_GOTFOCUS;
    }

    InvalidateRect(hwnd, NULL, FALSE);

    return 0;
}


static BOOL
DATETIME_SendDateTimeChangeNotify (HWND hwnd)

{
 DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);
 NMDATETIMECHANGE dtdtc;

 TRACE ("\n");
 dtdtc.nmhdr.hwndFrom = hwnd;
 dtdtc.nmhdr.idFrom   = GetWindowLongA( hwnd, GWL_ID);
 dtdtc.nmhdr.code     = DTN_DATETIMECHANGE;

 if ((GetWindowLongA (hwnd, GWL_STYLE) & DTS_SHOWNONE))
   dtdtc.dwFlags = GDT_NONE;
 else
   dtdtc.dwFlags = GDT_VALID;

 MONTHCAL_CopyTime (&infoPtr->date, &dtdtc.st);
 return (BOOL) SendMessageA (infoPtr->hwndNotify, WM_NOTIFY,
                              (WPARAM)dtdtc.nmhdr.idFrom, (LPARAM)&dtdtc);
}


static BOOL
DATETIME_SendSimpleNotify (HWND hwnd, UINT code)
{
    DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);
    NMHDR nmhdr;

    TRACE("%x\n",code);
    nmhdr.hwndFrom = hwnd;
    nmhdr.idFrom   = GetWindowLongA( hwnd, GWL_ID);
    nmhdr.code     = code;

    return (BOOL) SendMessageA (infoPtr->hwndNotify, WM_NOTIFY,
                                (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);
}

static LRESULT
DATETIME_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr(hwnd);
  DWORD dwStyle = GetWindowLongA(hwnd, GWL_STYLE);

  /* set size */
  infoPtr->rcClient.bottom = HIWORD(lParam);
  infoPtr->rcClient.right = LOWORD(lParam);

  TRACE("Height=%ld, Width=%ld\n", infoPtr->rcClient.bottom, infoPtr->rcClient.right);

  /* use DrawEdge to adjust the size of rcEdge to get rcDraw */
  memcpy((&infoPtr->rcDraw), (&infoPtr->rcClient), sizeof(infoPtr->rcDraw));

  DrawEdge(NULL, &(infoPtr->rcDraw), EDGE_SUNKEN, BF_RECT | BF_ADJUST);

  /* set the size of the button that drops the calendar down */
  /* FIXME: account for style that allows button on left side */
  infoPtr->calbutton.top   = infoPtr->rcDraw.top;
  infoPtr->calbutton.bottom= infoPtr->rcDraw.bottom;
  infoPtr->calbutton.left  = infoPtr->rcDraw.right-15;
  infoPtr->calbutton.right = infoPtr->rcDraw.right;

  /* set enable/disable button size for show none style being enabled */
  /* FIXME: these dimensions are completely incorrect */
  infoPtr->checkbox.top = infoPtr->rcDraw.top;
  infoPtr->checkbox.bottom = infoPtr->rcDraw.bottom;
  infoPtr->checkbox.left = infoPtr->rcDraw.left;
  infoPtr->checkbox.right = infoPtr->rcDraw.left + 10;

  /* update the position of the monthcal control */
  if(dwStyle & DTS_RIGHTALIGN)
    infoPtr->monthcal_pos.x = infoPtr->rcClient.right - ((infoPtr->calbutton.right -
                                infoPtr->calbutton.left) + 145);
  else
    infoPtr->monthcal_pos.x = 8;

  infoPtr->monthcal_pos.y = infoPtr->rcClient.bottom;
  ClientToScreen (hwnd, &(infoPtr->monthcal_pos));
  SetWindowPos(infoPtr->hMonthCal, 0, infoPtr->monthcal_pos.x,
    infoPtr->monthcal_pos.y,
    145, 150, 0);

  InvalidateRect(hwnd, NULL, FALSE);

  return 0;
}


static LRESULT
DATETIME_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  DATETIME_INFO *infoPtr;
  DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);

  /* allocate memory for info structure */
  TRACE("%04x %08lx\n",wParam,lParam);
  infoPtr = (DATETIME_INFO *)Alloc (sizeof(DATETIME_INFO));
  if (infoPtr == NULL) {
    ERR("could not allocate info memory!\n");
    return 0;
  }

  SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

  if (dwStyle & DTS_SHOWNONE) {
    infoPtr->hwndCheckbut=CreateWindowExA (0,"button", 0,
         WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
         2,2,13,13,
         hwnd,
         0, (HINSTANCE)GetWindowLongA  (hwnd, GWL_HINSTANCE), 0);
         SendMessageA (infoPtr->hwndCheckbut, BM_SETCHECK, 1, 0);
  }

  if (dwStyle & DTS_UPDOWN) {
    infoPtr->hUpdown=CreateUpDownControl (
        WS_CHILD | WS_BORDER | WS_VISIBLE,
        120,1,20,20,
        hwnd,1,0,0,
        UD_MAXVAL, UD_MINVAL, 0);
  }

  infoPtr->fieldspec = (int *) Alloc (32*sizeof(int));
  infoPtr->fieldRect = (RECT *) Alloc (32*sizeof(RECT));
  infoPtr->buflen = (int *) Alloc (32*sizeof(int));
  infoPtr->nrFieldsAllocated = 32;
  infoPtr->hwndNotify = ((LPCREATESTRUCTA)lParam)->hwndParent;

  DATETIME_SetFormat (hwnd, 0, 0);

  /* create the monthcal control */
    infoPtr->hMonthCal = CreateWindowExA (0,"SysMonthCal32", 0,
	WS_BORDER | WS_POPUP | WS_CLIPSIBLINGS,
	0, 0, 0, 0,
	hwnd,
	0, 0, 0);

  /* initialize info structure */
  GetSystemTime (&infoPtr->date);
  infoPtr->dateValid = TRUE;
  infoPtr->hFont = GetStockObject(DEFAULT_GUI_FONT);
  return 0;
}


static LRESULT
DATETIME_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DATETIME_INFO *infoPtr = DATETIME_GetInfoPtr (hwnd);

    TRACE("\n");
    if (infoPtr->hMonthCal) {
	/* FIXME I don't completely understand why doesn't it
	 *   happen automatically, WS_POPUP maybe? */
        DestroyWindow(infoPtr->hMonthCal);
	infoPtr->hMonthCal = NULL;
    }
    Free (infoPtr);
    SetWindowLongA( hwnd, 0, 0 ); /* clear infoPtr */
    return 0;
}


static LRESULT WINAPI
DATETIME_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!DATETIME_GetInfoPtr(hwnd) && (uMsg != WM_CREATE))
	return DefWindowProcA( hwnd, uMsg, wParam, lParam );

    switch (uMsg)
    {

    case DTM_GETSYSTEMTIME:
        return DATETIME_GetSystemTime (hwnd, wParam, lParam);

    case DTM_SETSYSTEMTIME:
	return DATETIME_SetSystemTime (hwnd, wParam, lParam);

    case DTM_GETRANGE:
        return DATETIME_GetRange(hwnd, lParam);

    case DTM_SETRANGE:
        return DATETIME_SetRange(hwnd, wParam, lParam);

    case DTM_SETFORMATA:
        return DATETIME_SetFormat (hwnd, wParam, lParam);

    case DTM_SETFORMATW:
        return DATETIME_SetFormatW (hwnd, wParam, lParam);

    case DTM_SETMCCOLOR:
        return DATETIME_SetMonthCalColor (hwnd, wParam, lParam);

    case DTM_GETMCCOLOR:
        return DATETIME_GetMonthCalColor (hwnd, wParam);

    case DTM_GETMONTHCAL:
        return DATETIME_GetMonthCal (hwnd);

    case DTM_SETMCFONT:
        return DATETIME_SetMonthCalFont (hwnd, wParam, lParam);

    case DTM_GETMCFONT:
        return DATETIME_GetMonthCalFont (hwnd);

    case WM_PARENTNOTIFY:
	return DATETIME_ParentNotify (hwnd, wParam, lParam);

    case WM_NOTIFY:
	return DATETIME_Notify (hwnd, wParam, lParam);

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;

    case WM_PAINT:
        return DATETIME_Paint (hwnd, wParam);

    case WM_KEYDOWN:
        return DATETIME_KeyDown (hwnd, wParam, lParam);

    case WM_KILLFOCUS:
        return DATETIME_KillFocus (hwnd, wParam, lParam);

    case WM_SETFOCUS:
        return DATETIME_SetFocus (hwnd, wParam, lParam);

    case WM_SIZE:
        return DATETIME_Size (hwnd, wParam, lParam);

    case WM_LBUTTONDOWN:
        return DATETIME_LButtonDown (hwnd, wParam, lParam);

    case WM_LBUTTONUP:
        return DATETIME_LButtonUp (hwnd, wParam, lParam);

    case WM_CREATE:
	return DATETIME_Create (hwnd, wParam, lParam);

    case WM_DESTROY:
	return DATETIME_Destroy (hwnd, wParam, lParam);

    case WM_COMMAND:
        return DATETIME_Command (hwnd, wParam, lParam);

    default:
	if ((uMsg >= WM_USER) && (uMsg < WM_APP))
		ERR("unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
DATETIME_Register (void)
{
    WNDCLASSA wndClass;

    TRACE("\n");
    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC)DATETIME_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(DATETIME_INFO *);
    wndClass.hCursor       = LoadCursorA (0, (LPSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = DATETIMEPICK_CLASSA;

    RegisterClassA (&wndClass);
}


VOID
DATETIME_Unregister (void)
{
    TRACE("\n");
    UnregisterClassA (DATETIMEPICK_CLASSA, NULL);
}
