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

#define SERVERLISTSIZE 6
#define BUFSIZE 1024
#define MYPORT 6
#define NTPPORT 6
#define ID_TIMER 1

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

BOOL InitClockWindowClass();

BOOL InitialiseConnection(VOID);
VOID DestroyConnection(VOID);
BOOL SendData(VOID);
BOOL RecieveData(CHAR *);

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
