#ifndef __INTL_H
#define __INTL_H

/* GLOBALS ******************************************************************/

#define MAX_STR_SIZE          128
#define MAX_FMT_SIZE          30
#define MAX_SAMPLES_STR_SIZE  70
#define DECIMAL_RADIX         10

typedef struct
{
    int idIcon;
    int idName;
    int idDescription;
    APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

/* locale.c */
INT_PTR
CALLBACK
RegOptsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* extra.c */
INT_PTR
CALLBACK
ExtraOptsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* langs.c */
INT_PTR
CALLBACK
LangsOptsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* date.c */
INT_PTR
CALLBACK
DateOptsSetProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* nums.c */
INT_PTR
CALLBACK
NumsOptsSetProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* monetary.c */
INT_PTR
CALLBACK
CurrencyOptsSetProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* time.c */
INT_PTR
CALLBACK
TimeOptsSetProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* setupreg.c */
INT_PTR
CALLBACK
RegOptsSetProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

WCHAR*
ReplaceSubStr(const WCHAR *wszSourceStr, const WCHAR *wszStrToReplace, const WCHAR *wszTempl);

WCHAR*
InsSpacesFmt(const WCHAR *wszSourceStr, const WCHAR *wszFmtStr);

LONG
APIENTRY
SetupApplet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam);

#endif /* __INTL_H */

/* EOF */
