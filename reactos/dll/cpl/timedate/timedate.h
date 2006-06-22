#ifndef __CPL_SAMPLE_H
#define __CPL_SAMPLE_H

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdio.h>
#include <winsock2.h>
#include <math.h>
#include <commctrl.h>
#include <cpl.h>

#include "resource.h"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
#define SERVERLISTSIZE 6
#define BUFSIZE 1024
#define NTPPORT 123
#define ID_TIMER 1

typedef struct
{
  UINT idIcon;
  UINT idName;
  UINT idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;


/* dateandtime.c */
INT_PTR CALLBACK DateTimePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


/* timezone.c */
INT_PTR CALLBACK TimeZonePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


/* internettime.c */
INT_PTR CALLBACK InetTimePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


/* timedate.c */
VOID GetError(VOID);


/* clock.c */
BOOL RegisterClockControl(VOID);
VOID UnregisterClockControl(VOID);


/* ntpclient.c */
BOOL InitialiseConnection(CHAR *szIpAddr);
VOID DestroyConnection(VOID);
BOOL SendData(VOID);
ULONG RecieveData(VOID);


/* monthcal.c */
#define MCCM_SETDATE    (WM_USER + 1)
#define MCCM_GETDATE    (WM_USER + 2)
#define MCCM_RESET      (WM_USER + 3)
#define MCCM_CHANGED    (WM_USER + 4)

#define MCCN_SELCHANGE   (1)
typedef struct _NMMCCSELCHANGE
{
    NMHDR hdr;
    WORD OldDay;
    WORD OldMonth;
    WORD OldYear;
    WORD NewDay;
    WORD NewMonth;
    WORD NewYear;
} NMMCCSELCHANGE, *PNMMCCSELCHANGE;
#define MCCN_AUTOUPDATE (2)
typedef struct _NMMCCAUTOUPDATE
{
    NMHDR hdr;
    SYSTEMTIME SystemTime;
} NMMCCAUTOUPDATE, *PNMMCCAUTOUPDATE;

BOOL RegisterMonthCalControl(IN HINSTANCE hInstance);
VOID UnregisterMonthCalControl(IN HINSTANCE hInstance);

#endif /* __CPL_SAMPLE_H */

/* EOF */
