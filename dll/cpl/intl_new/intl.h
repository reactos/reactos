#ifndef __INTL_H
#define __INTL_H

/* GLOBALS ******************************************************************/

#define MAX_STR_SIZE    128
#define MAX_FMT_SIZE    30

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

#endif /* __INTL_H */

/* EOF */
