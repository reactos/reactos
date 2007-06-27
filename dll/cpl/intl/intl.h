#ifndef __CPL_INTL_H
#define __CPL_INTL_H


typedef struct _APPLET
{
  UINT idIcon;
  UINT idName;
  UINT idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;


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

#endif /* __CPL_INTL_H */

/* EOF */
