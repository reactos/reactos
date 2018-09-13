/*** 
* Rtdate.c - Date/time support
*
*   Copyright (C) 1992, Microsoft Corporation.  All Rights Reserved.
*   Information Contained Herein Is Proprietary and Confidential.
*
* Purpose:
*   This source contains the date/time support that are exported
*   for use in the ole dlls.
*
* Revision History:
*
*    [00] 17-Dec-92 bradlo: Split off from rtdate.c
*
* Implementation Notes:
*
*****************************************************************************/

#include "oledisp.h"
#pragma hdrstop(RTPCHNAME)

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

ASSERTDATA


#define HALF_SECOND  (1.0/172800.0)

// Map from month number (0-based) to day of year month starts (0-based)
// Good for non-leap years. Last entry is 365 (days in a year)
//
const int NEARDATA mpmmdd[13] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

extern const double dblSecPerDay = 86400.0; // keep compiler from "optimizing" our
	  // divide by this constant to a multiply by
	  // it's recriprocal, which is less accurate.

// helper function to return the proper full year from a given 2-digit year
// current algorithm is: 0-29 ==> 20xx, 30-99 ==> 19xx
#define BaseYearOf2DigitYear(yy) ((yy <= 29) ? 2000 : 1900)

#if OE_MACPPC
// UNDONE: remove when we upgrade Mac PPC tools.
// See VBE bug #4065 for repro scenario
#pragma optimize("", off) // UNDONE: remove when tools are upgraded
#endif //OE_MACPPC

// CALENDAR_SUPPORT
STDAPI VarDateFromUdate(UDATE *pudateIn, unsigned long dwFlags, DATE *pdateOut)
{
    long lDate;
    double dblTime;
    int fLeapYear;
    short mm, dd, yy;
    UDATE udateTemp;
    HRESULT hr;

    // put uds stuff in local vars
    // also, make months zero based and adjust for out of range
    // months (for date calculations using DateSerial)

    if (VAR_CALENDAR_HIJRI & dwFlags) {
      udateTemp = *pudateIn;
      pudateIn = &udateTemp;
      hr = ErrConvertUdsCalendar((UDS *)pudateIn, CALENDAR_HIJRI, 
	CALENDAR_GREGORIAN, ((VAR_VALIDDATE & dwFlags) ? TRUE : FALSE));
      if (FAILED(hr))
	return hr;
    }

    yy = pudateIn->st.wYear;
    mm = pudateIn->st.wMonth - 1;

    if (!(VAR_VALIDDATE & dwFlags)) {
      if (mm < 0) {
	yy -= 1 + (-mm / 12);
	mm = 12 - (-mm % 12);
      }
      else {
	yy += mm / 12;
	mm = mm % 12;
      }
    }

    if (yy < 100) {
      // convert to the full year
      yy += BaseYearOf2DigitYear(yy);
    }

    dd = pudateIn->st.wDay; // UDATE->st.wDay corresponds to UDS->DayOfMonth

    /* Check for leap year */

    fLeapYear = ((yy & 3) == 0) && ((yy % 100) != 0 || (yy % 400) == 0);

    /* Check if it's a valid date */

    if (yy < 0 || yy > 9999 || mm < 0 || mm > 12)
      return RESULT(E_INVALIDARG);

    if ((VAR_VALIDDATE & dwFlags)
      && (mm > 11 || dd < 1 || dd > mpmmdd[mm + 1] - mpmmdd[mm])
      && !(mm == 1 && dd == 29 && fLeapYear))
    {
      return RESULT(E_INVALIDARG);
    }

    /* It is a valid date; make Jan 1, 1AD be 1 */

    lDate = yy * 365L + (yy / 4) - yy/100 + yy/400 + mpmmdd[mm] + dd;

    /* If we are a leap year and it's before March, subtract 1:
       we haven't leapt yet! */

    if(mm < 2 && fLeapYear)
      --lDate;

    /* Offset so that 12/30/1899 is 0 */

    lDate -= 693959L;

    if(lDate > 2958465 || lDate < -657434)
      return RESULT(E_INVALIDARG);

    if ((VAR_VALIDDATE & dwFlags)
     && (pudateIn->st.wHour > 23
      || pudateIn->st.wMinute > 59
      || pudateIn->st.wSecond > 59))
    {
      return RESULT(E_INVALIDARG);
    }

    dblTime = (((long)(short)pudateIn->st.wHour * 3600L) +  // hrs in seconds
	   ((long)(short)pudateIn->st.wMinute * 60L) +  // mins in seconds
	   ((long)(short)pudateIn->st.wSecond)) / dblSecPerDay;

    if (dwFlags & VAR_TIMEVALUEONLY)
      *pdateOut = dblTime;
    else if (dwFlags & VAR_DATEVALUEONLY)
      *pdateOut = (double)lDate;
    else
      *pdateOut = (double)lDate + ((lDate >= 0) ? dblTime : -dblTime);
    return NOERROR;
}

/***
*ErrPackDate - pack a UDS into a date/time serial number
*
*Purpose:
*  Generate a date/time serial number from a UDS
*
*Entry:
*  UDS *puds = pointer to UDS to pack
*  VARIANT *pvarDate = pointer to serial date to be filled
*  int fValidDate =
*    TRUE - the UDS is known to contain a valid date
*    FALSE - perform more checks and work to pack a possibly invalid date
*  dwFlags =
*    VAR_TIMEVALUEONLY - return only time value
*    VAR_DATEVALUEONLY - return only date value
*    VAR_CALENDAR_HIJRI - Use Hijri Calendar. CALENDAR_SUPPORT
*
*
*Exit:
*  return value = HRESULT
*
***********************************************************************/
EXTERN_C INTERNAL_(HRESULT)
ErrPackDate(UDS FAR* puds, VARIANT FAR* pvarDate, int fValidDate, unsigned long dwFlags)
{
    HRESULT hr;
    DATE dateWanted;
    
    if (fValidDate)
      dwFlags |= VAR_VALIDDATE;
      
    hr = VarDateFromUdate(&puds->udate, dwFlags, &dateWanted);
    if (SUCCEEDED(hr))
    {
      V_VT(pvarDate) = VT_DATE;
      V_DATE(pvarDate) = dateWanted;
    }
      
    return (hr);
}


//CALENDAR_SUPPORT
STDAPI VarUdateFromDate(DATE dateIn, unsigned long dwFlags, UDATE *pudateOut)
{
    int mm;
    int nTime;
    int fLeapOly;
    int const * lpdd;
    double serialDate, roundDate;
    long lDate, lTime;
    int oly, nDate, y, c4, cty;

    fLeapOly = TRUE; // assume true

    roundDate = serialDate = dateIn;

    /* Local variables:
     *
     *  lDate   day based on 0 as 1/1/0 (note different from input)
     *    so 1/1/1900 is 693961 under this system.
     *  lTime   time in seconds since midnight
     *  c4  number of 400-year blocks since 1/1/0
     *  cty century within c4 (0-3)
     *  oly olympiad (starting with oly 0 is years AD 0-3)
     *  nDate   day number within oly (0 is 1/1 of 1st year;
     *    1460 is 12/31 of 4th)
     *  y year in oly (0-3)
     *  fLeapOly true if the date is in a leap oly
     */

    /* WARNING: DO NOT CAST serialDate TO A 'long' BEFORE VERIFYING
      THAT IT WILL NOT CAUSE A FLOATING-POINT EXCEPTION DUE TO
      OVERFLOW.  _aFftol IS NOT FOLLOWED BY AN FWAIT INSTRUCTION,
      SO THE FP EXCEPTION WON'T BE RAISED UNTIL SOME OTHER CODE
      CALLS FWAIT.  ErrUnpackDate() CANNOT RAISE EXCEPTIONS - IT
      MUST RETURN AN ERROR CODE.    VBA2 #1914
    */
    if(serialDate >= 2958466.0 || serialDate <= -657435.0)
      return RESULT(E_INVALIDARG);

    // prep for round to the sec
    roundDate += (serialDate > 0.0) ? HALF_SECOND : -HALF_SECOND;

    if(roundDate <= 2958465.0 && roundDate >= -657435.0)
      serialDate = roundDate;

    lDate = (long)serialDate + 693959L; // Add days from 1/1/0 to 12/30/1899

    if(serialDate < 0)
      serialDate = -serialDate;

    lTime = (long)((serialDate - floor(serialDate)) * 86400.);

    if (lDate < 0)
      lDate = 0;

    pudateOut->st.wDayOfWeek = (short) ((lDate-1) % 7L); // -1 because 1/1/0 is Sat.

    /* Leap years are every 4 years except centuries that are
       not multiples of 400. */

    c4 = (int)(lDate / 146097L);

    // Now lDate is day within 400-year block
    lDate %= 146097L;

    // -1 because first century has extra day
    cty = (int)((lDate - 1) / 36524L);

    if(cty != 0){   // Non-leap century

      // Now lDate is day within century
      lDate = (lDate - 1) % 36524L;

      // +1 because 1st oly has 1460 days
      oly = (int) ((lDate + 1) / 1461L);

      if(oly != 0){
	nDate = (int)((lDate + 1) % 1461L);
      }
      else{
	fLeapOly = FALSE;
	nDate = (int)lDate;
      }
    } 
    else {    // Leap century - not special case!
      oly = (int)(lDate / 1461L);
      nDate = (int)(lDate % 1461L);
    }

    if (fLeapOly) {
      y = (nDate - 1) / 365;  // -1 because first year has 366 days
      if(y != 0)
	nDate = (nDate - 1) % 365;
    } 
    else {
      y = nDate / 365;
      nDate %= 365;
    }

    // nDate is now 0-based day of year. Save 1-based day of year, year number

    pudateOut->wDayOfYear = nDate + 1;
    pudateOut->st.wYear = c4 * 400 + cty * 100 + oly * 4 + y;

    // Handle leap year: before, on, and after Feb. 29.

    if (y == 0 && fLeapOly) { // Leap Year

      if (nDate == 59) {
	/* Feb. 29 */
	pudateOut->st.wMonth = 2;
	pudateOut->st.wDay = 29; // UDATE->st.wDay corresponds to UDS->DayOfMonth
	goto DoTime;
      }

      if (nDate >= 60)
	--nDate; // Pretend it's not a leap year for month/day comp.
    }

    // Code from here to DoTime computes month/day for everything but Feb. 29.

    ++nDate;  /* Make it 1-based rather than 0-based */

    // nDate is now 1-based day of non-leap year.

    /* Month number will always be >= n/32, so save some loop time */

    for (mm = (nDate >> 5) + 1, lpdd = &mpmmdd[mm]; nDate > *lpdd; mm++, lpdd++);

    pudateOut->st.wMonth = (short)mm;
    pudateOut->st.wDay = (short)(nDate - lpdd[-1]); // UDATE->st.wDay corresponds to UDS->DayOfMonth

DoTime:;

    // We have all date info in uds.

    if(lTime == 0){   // avoid the divisions

      pudateOut->st.wSecond = pudateOut->st.wMinute = pudateOut->st.wHour = 0;

    }else{
      pudateOut->st.wSecond = (short)(lTime % 60L);
      nTime = (int)(lTime / 60L);
      pudateOut->st.wMinute = (short)(nTime % 60);
      pudateOut->st.wHour = (short)(nTime / 60);
    }

    
    if (VAR_CALENDAR_HIJRI & dwFlags)
      {
// some special handling for 31-Dec-1899 00:00:00 hrs.
// UNDONE What is this?   
//    if(!(pudateOut->st.wMonth == 12 && pudateOut->st.wDay == 30 && pudateOut->st.wYear == 1899) // UDATE->st.wDay corresponds to UDS->DayOfMonth
//       || !(pudateOut->st.wHour || pudateOut->st.wMinute || pudateOut->st.wSecond ))
	ErrConvertUdsCalendar((UDS *)pudateOut, CALENDAR_GREGORIAN, CALENDAR_HIJRI, TRUE);
      }

    return RESULT(NOERROR);
}

/***
* ErrUnpackDate - unpack a date/time serial number into a UDS
*
* Purpose:
*   Generate a UDS from a packed date/time serial number
*
* Entry:
*   UDS *puds = pointer to UDS to unpack into
*   double serialDate = serial number to unpack
*
*  dwFlags =
*    VAR_CALENDAR_HIJRI - Use Hijri Calendar. CALENDAR_SUPPORT
* 
*
* Exit:
*   EBERR_None = *puds is the unpacked date
*   EBERR_IllegalFuncCall = serialDate is out-of-range
*
* Exceptions:
*
***********************************************************************/
// CALENDAR_SUPPORT
EXTERN_C INTERNAL_(HRESULT)
ErrUnpackDate(UDS FAR* puds, VARIANT FAR* pvar, unsigned long dwFlags)
{
    if(V_VT(pvar) != VT_DATE && V_VT(pvar) != VT_R8)
      return RESULT(E_INVALIDARG);
    return (VarUdateFromDate(V_DATE(pvar), dwFlags, &puds->udate));
}

/***
* int GetCurrentYear - returns the current year
*
* Purpose:
*   Get the current year
*
* Entry:
*   None
*
* Exit:
*   returns current year
*
*
* Exceptions:
*
***********************************************************************/

#if OE_MACPPC
// UNDONE: remove when we upgrade Mac PPC tools.
// See VBE bug #4065 for repro scenario
#pragma optimize("", on)  // UNDONE: remove when tools are upgraded
#endif //OE_MACPPC

INTERNAL_(int)
GetCurrentYear(void)
{
#if OE_MAC

    DateTimeRec d;
    unsigned long secs;

    GetDateTime(&secs);
    Secs2Date(secs, &d);
    return (int)d.year;

#elif OE_WIN32

    SYSTEMTIME s;

    GetLocalTime(&s);
    return s.wYear;

#else
    void PASCAL DOS3CALL(void);
    int iYear;

    _asm {
      mov   ah, 0x2a      // 2a = Get Date Function
      call  DOS3CALL      // Int 21 call
      mov   iYear, cx
    }

    return iYear;
#endif
}

STDAPI GetAltMonthNames(LCID lcid, LPOLESTR * *prgp)
{
    switch (PRIMARYLANGID(LANGIDFROMLCID(lcid)))
    {
    case LANG_ARABIC: // 0x01
      {
      *prgp = (LPOLESTR *) g_rgszHijriMonth2;
      break;
      }
    case LANG_POLISH : // 0x15
      {
      *prgp = (LPOLESTR *) g_rgszPolishMonth2;
      break;
      }
    case LANG_RUSSIAN : // 0x19
      {
      *prgp = (LPOLESTR *) g_rgszRussianMonth2;
      break;
      }
    default :
      {
      *prgp = (LPOLESTR * ) NULL;
      break;
      }
    }
    return (RESULT(NOERROR)); 
}

// Hijri Date support stuff (for Middle East countries)
// Authors:
//   MRashid : Prototype Design and Implementation
//   MakAG   : Design reviews/modifications and Final Implementation

static int HDMY2nDays(int d, int m, int y);
static int HM2Days(int m);
static int HY2nDays(int y);
static int IsHYLeap(int y);
static int GY2nDays(int y);
static int IsGYLeap(int y);
static int GDMY2nDays(int d, int m, int y);
static int nDays2HY(int d);
static int nDays2HM(int ndays);
static int nDays2HD(int ndays);
static int nDays2GD(int ndays);
static int nDays2GM(int ndays);
static int nDays2GY(int days);
static int IsnDaysLeap(int days);
static int GM2Days(int m);

INTERNAL_(HRESULT) ErrConvertUdsCalendar(UDS * puds, int iCalendarFrom, int iCalendarTo, BOOL fValidDate)
    {
#define BASE_YEAR_HIJRI 1400
#define MAX_YEAR  9666
    long lDate;
	
    if (puds->Year < 0)
	return(RESULT(E_INVALIDARG));
	
    if (puds->Year < 100) {
      puds->Year += ((CALENDAR_GREGORIAN == iCalendarFrom) ?
	 BaseYearOf2DigitYear(puds->Year) : BASE_YEAR_HIJRI);
    }

    if ((CALENDAR_GREGORIAN == iCalendarTo) && (CALENDAR_HIJRI == iCalendarFrom)) {

      if (!fValidDate)
      {
	int mm = puds->Month - 1;
	int yy = puds->Year;
	if (mm < 0) {
	  yy -= 1 + (-mm / 12);
	  mm = 12 -  (-mm % 12);
	}
	else {
	  yy += mm / 12;
	  mm = mm % 12;
	}
	mm++;
	if(fValidDate) {
	  if( (puds->DayOfMonth > 30) || 
	    (puds->DayOfMonth < 1) || 
	    (mm > 12) || 
	    (mm < 1) || 
	    (yy > MAX_YEAR) || 
	    (yy <= 0))
	  return(RESULT(E_INVALIDARG));
	}
	puds->Month = mm;    
	puds->Year = yy;     
      } 
      lDate = HDMY2nDays(puds->DayOfMonth, puds->Month, puds->Year);
    
      puds->DayOfMonth    = nDays2GD(lDate);
      puds->Month = nDays2GM(lDate);
      puds->Year    = nDays2GY(lDate);
    
      lDate = GDMY2nDays(puds->DayOfMonth, puds->Month, puds->Year);
      
      puds->DayOfYear = lDate - GY2nDays(puds->Year);
      return (RESULT(NOERROR));

    }
    else if ((CALENDAR_HIJRI == iCalendarTo) && (CALENDAR_GREGORIAN == iCalendarFrom))
    {
      lDate = GDMY2nDays(puds->DayOfMonth, puds->Month, puds->Year);
    
      puds->Month = nDays2HM(lDate);
      puds->Year  = nDays2HY(lDate);
      puds->DayOfMonth   = nDays2HD(lDate);
      
      lDate = HDMY2nDays(puds->DayOfMonth, puds->Month, puds->Year);
      
      puds->DayOfYear = lDate - HY2nDays(puds->Year);
      return (RESULT(NOERROR));
    }
    return (RESULT(E_INVALIDARG));
}

static int HDMY2nDays(int d, int m, int y)
{
    return (int)(HY2nDays(y) + HM2Days(m) + d);
}

static int HM2Days(int m)
{
    int mdays[13] = {0,30,59,89,118,148,177,207,236,266,295,325,355};

    return (mdays[m-1]);
}

static int HY2nDays(int y)
{   
    int y30, yleft;
    int result;

    y30   = ((y-1) / 30) * 30;
    yleft = y - y30 - 1;
    result  = (((int)y30 * 10631L) / 30L) + 227013L;
    while (yleft > 0)
      result += 354L + (int)IsHYLeap(yleft--);

    return result;
}

static int IsHYLeap(int y)
{
    int mdays[11]={2,5,7,10,13,15,18,21,24,26,29};
    int i;

    y %= 30;
    for (i=0; i<11; i++) {
      if (y == mdays[i])
	return 1;
    } 
    return 0;
}

static int GY2nDays(int y)
{
    y = y - 1;
    return (int)((int)y*365L + y/4 - y/100 + y/400);
}


static int IsGYLeap(int y)
{
    return ((y%4 == 0) && ((y%100 > 0) || (y%400 == 0)));
}

static int GDMY2nDays(int d, int m, int y)
{   
    return (int)((IsGYLeap(y) && (m > 2)) + GY2nDays(y) + GM2Days(m) + d);
}

static int nDays2HY(int d)
{
    int hy = (int)(((d - 227013L) * 30L / 10631L) + 1);

    if (d <= HY2nDays(hy))
       hy--;
    else if (d > HY2nDays(hy+1))
       hy++;
    return hy;
}

static int nDays2HM(int ndays)
{
    int i = 1;
    int d = (int)(ndays - HY2nDays(nDays2HY(ndays)));
    while (d > HM2Days(i))
      i++;
    return (i - 1);
}

static int nDays2HD(int ndays)
{
    return (int)(ndays - HY2nDays(nDays2HY(ndays)) - HM2Days(nDays2HM(ndays)));
}

static int nDays2GD(int ndays)
{
    return (int)(ndays - GDMY2nDays(1, nDays2GM(ndays), nDays2GY(ndays)) + 1);
}

static int nDays2GM(int ndays)
{
    int i = 1;
    int d = (int)(ndays - GY2nDays(nDays2GY(ndays)) - IsnDaysLeap(ndays));

    while (d > GM2Days(i))
      i++;
    return (i - 1);
}

static int nDays2GY(int days)
{
    int d = (int)(days * 400L / 146097L);

    if (days > GY2nDays(d+2))
      return (d+2);
    else if (days > GY2nDays(d+1))
      return (d+1);
    else
      return (d);
}

static int IsnDaysLeap(int days)
{
    int y = nDays2GY(days);

    return (IsGYLeap(y) && (days - GY2nDays(y) > 59));
}

static int GM2Days(int m)
{
    int mdays[13] = {0,31,59,90,120,151,181,212,243,273,304,334,365};

    return mdays[m-1];
}

