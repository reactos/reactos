#ifndef __CPL_INTL_H
#define __CPL_INTL_H

#define MAX_FMT_SIZE          30
#define MAX_STR_SIZE          128
#define MAX_SAMPLES_STR_SIZE  70
#define DECIMAL_RADIX         10

typedef struct _APPLET
{
  UINT idIcon;
  UINT idName;
  UINT idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;
extern DWORD IsUnattendedSetupEnabled;
extern DWORD UnattendLCID;

/* intl.c */
VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc);

/* languages.c */
INT_PTR CALLBACK
LanguagesPageProc(HWND hwndDlg,
	     UINT uMsg,
	     WPARAM wParam,
	     LPARAM lParam);
		 
/* advanced.c */
INT_PTR CALLBACK
AdvancedPageProc(HWND hwndDlg,
	     UINT uMsg,
	     WPARAM wParam,
	     LPARAM lParam);

/* currency.c */
INT_PTR CALLBACK
CurrencyPageProc(HWND hwndDlg,
		 UINT uMsg,
		 WPARAM wParam,
		 LPARAM lParam);

/* date.c */
INT_PTR CALLBACK
DatePageProc(HWND hwndDlg,
	     UINT uMsg,
	     WPARAM wParam,
	     LPARAM lParam);

/* general.c */
INT_PTR CALLBACK
GeneralPageProc(HWND hwndDlg,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam);

/* locale.c */
INT_PTR CALLBACK
InpLocalePageProc(HWND hwndDlg,
	       UINT uMsg,
	       WPARAM wParam,
	       LPARAM lParam);

/* numbers.h */
INT_PTR CALLBACK
NumbersPageProc(HWND hwndDlg,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam);

/* time.c */
INT_PTR CALLBACK
TimePageProc(HWND hwndDlg,
	     UINT uMsg,
	     WPARAM wParam,
	     LPARAM lParam);


void SetNewLocale(LCID lcid);

/* misc.c */
LPTSTR
InsSpacesFmt(LPCTSTR szSourceStr, LPCTSTR szFmtStr);

LPTSTR
ReplaceSubStr(LPCTSTR szSourceStr, LPCTSTR szStrToReplace, LPCTSTR szTempl);

LONG
APIENTRY
SetupApplet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam);

#endif /* __CPL_INTL_H */

/* EOF */
