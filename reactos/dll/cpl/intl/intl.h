#ifndef _INTL_H
#define _INTL_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <winuser.h>
#include <cpl.h>
#include <setupapi.h>
#include <tchar.h>
#include <malloc.h>
#include <ndk/exfuncs.h>

#include "resource.h"

#define DECIMAL_RADIX          10

/* Limits */
#define MAX_FMT_SIZE           30
#define MAX_STR_SIZE          128
#define MAX_SAMPLES_STR_SIZE   70

#define MAX_NUMBERDSYMBOL       3
#define MAX_NUMBERSDIGITGRSYM   3
#define MAX_NUMBERSNSIGNSYM     4
#define MAX_NUMBERSLSEP         3

#define MAX_CURRENCYSYMBOL      5
#define MAX_CURRENCYDECSEP      3
#define MAX_CURRENCYGRPSEP      3

#define MAX_TIMEFORMAT         80
#define MAX_TIMESEPARATOR       3
#define MAX_TIMEAMSYMBOL       12
#define MAX_TIMEPMSYMBOL       12

#define MAX_SHRTDATEFMT        80
#define MAX_SHRTDATESEP         3
#define MAX_LONGDATEFMT        80
#define MAX_YEAR_EDIT           4

typedef struct _APPLET
{
    UINT idIcon;
    UINT idName;
    UINT idDescription;
    APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

typedef struct _GLOBALDATA
{
    LCID lcid;
} GLOBALDATA, *PGLOBALDATA;

extern HINSTANCE hApplet;
extern DWORD IsUnattendedSetupEnabled;
extern DWORD UnattendLCID;

/* intl.c */
VOID PrintErrorMsgBox(UINT msg);

/* languages.c */
INT_PTR CALLBACK
LanguagesPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* advanced.c */
INT_PTR CALLBACK
AdvancedPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID
SetNonUnicodeLang(HWND hwnd, LCID lcid);

/* currency.c */
INT_PTR CALLBACK
CurrencyPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* date.c */
INT_PTR CALLBACK
DatePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* general.c */
INT_PTR CALLBACK
GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
LPTSTR
InsSpacesFmt(LPCTSTR szSourceStr, LPCTSTR szFmtStr);

LPTSTR
ReplaceSubStr(LPCTSTR szSourceStr, LPCTSTR szStrToReplace, LPCTSTR szTempl);

LONG
APIENTRY
SetupApplet(HWND hwndDlg, LCID lcid);

/* kblayouts.c */
VOID AddNewKbLayoutsByLcid(LCID Lcid);

#endif /* _INTL_H */
