#ifndef _INTL_H
#define _INTL_H

#include <stdarg.h>
#include <stdlib.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <winuser.h>
#include <cpl.h>
#include <setupapi.h>
#include <malloc.h>
#include <ndk/exfuncs.h>

#include "resource.h"

#define DECIMAL_RADIX          10

/* Limits */
#define MAX_FMT_SIZE           30
#define MAX_STR_SIZE          128
#define MAX_SAMPLES_STR_SIZE   70

#define MAX_NUMDECIMALSEP       4
#define MAX_NUMTHOUSANDSEP      4
#define MAX_NUMNEGATIVESIGN     5
#define MAX_NUMPOSITIVESIGN     5
#define MAX_NUMLISTSEP          4
#define MAX_NUMNATIVEDIGITS    11

#define MAX_CURRSYMBOL         13
#define MAX_CURRDECIMALSEP      4
#define MAX_CURRTHOUSANDSEP     4
#define MAX_CURRGROUPING       10

#define MAX_TIMEFORMAT         80
#define MAX_TIMESEPARATOR       4
#define MAX_TIMEAMSYMBOL       15
#define MAX_TIMEPMSYMBOL       15

#define MAX_SHORTDATEFORMAT    80
#define MAX_LONGDATEFORMAT     80
#define MAX_DATESEPARATOR       4
#define MAX_YEAR_EDIT           4

#define MAX_MISCCOUNTRY        80
#define MAX_MISCLANGUAGE       80


typedef struct _APPLET
{
    UINT idIcon;
    UINT idName;
    UINT idDescription;
    APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

typedef struct _GLOBALDATA
{
    /* Number */
    WCHAR szNumDecimalSep[MAX_NUMDECIMALSEP];
    WCHAR szNumThousandSep[MAX_NUMTHOUSANDSEP];
    WCHAR szNumNegativeSign[MAX_NUMNEGATIVESIGN];
    WCHAR szNumPositiveSign[MAX_NUMPOSITIVESIGN];
    WCHAR szNumListSep[MAX_NUMLISTSEP];
    WCHAR szNumNativeDigits[MAX_NUMNATIVEDIGITS];
    INT nNumNegFormat;
    INT nNumDigits;
    INT nNumLeadingZero;
    INT nNumGrouping;
    INT nNumMeasure;
    INT nNumShape;

    /* Currency */
    WCHAR szCurrSymbol[MAX_CURRSYMBOL];
    WCHAR szCurrDecimalSep[MAX_CURRDECIMALSEP];
    WCHAR szCurrThousandSep[MAX_CURRTHOUSANDSEP];
    INT nCurrPosFormat;
    INT nCurrNegFormat;
    INT nCurrDigits;
    INT nCurrGrouping;

    /* Time */
    WCHAR szTimeFormat[MAX_TIMEFORMAT];
    WCHAR szTimeSep[MAX_TIMESEPARATOR];
    WCHAR szTimeAM[MAX_TIMEAMSYMBOL];
    WCHAR szTimePM[MAX_TIMEPMSYMBOL];
    INT nTime;
    INT nTimePrefix;
    INT nTimeLeadingZero;

    /* Date */
    WCHAR szLongDateFormat[MAX_LONGDATEFORMAT];
    WCHAR szShortDateFormat[MAX_SHORTDATEFORMAT];
    WCHAR szDateSep[MAX_DATESEPARATOR];
    INT nFirstDayOfWeek;
    INT nFirstWeekOfYear;
    INT nDate;
    INT nCalendarType;

    /* Other */
    WCHAR szMiscCountry[MAX_MISCCOUNTRY];
    WCHAR szMiscLanguage[MAX_MISCLANGUAGE];
    INT nMiscCountry;

    LCID UserLCID;
    BOOL fUserLocaleChanged;
    BOOL bApplyToDefaultUser;

    GEOID geoid;
    BOOL fGeoIdChanged;

} GLOBALDATA, *PGLOBALDATA;

extern HINSTANCE hApplet;
extern DWORD IsUnattendedSetupEnabled;
extern DWORD UnattendLCID;

/* intl.c */
VOID PrintErrorMsgBox(UINT msg);

VOID
ResourceMessageBox(
    HWND hwnd,
    UINT uType,
    UINT uCaptionId,
    UINT uMessageId);

/* languages.c */
INT_PTR CALLBACK
LanguagesPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* advanced.c */
INT_PTR CALLBACK
AdvancedPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* currency.c */
INT_PTR CALLBACK
CurrencyPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* date.c */
INT_PTR CALLBACK
DatePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* general.c */
INT_PTR CALLBACK
GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

PWSTR
GetLocaleString(
    PWSTR *pLocaleArray,
    LCTYPE lcType);

/* locale.c */
INT_PTR CALLBACK
InpLocalePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* numbers.h */
INT_PTR CALLBACK
NumbersPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* time.c */
INT_PTR CALLBACK
TimePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* sort.c */
BOOL
IsSortPageNeeded(LCID lcid);

INT_PTR CALLBACK
SortPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* misc.c */
PWSTR
InsSpacesFmt(PCWSTR szSourceStr, PCWSTR szFmtStr);

PWSTR
ReplaceSubStr(PCWSTR szSourceStr, PCWSTR szStrToReplace, PCWSTR szTempl);

/* kblayouts.c */
VOID AddNewKbLayoutsByLcid(LCID Lcid);

#endif /* _INTL_H */
