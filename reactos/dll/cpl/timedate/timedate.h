#ifndef __CPL_SAMPLE_H
#define __CPL_SAMPLE_H

#include <windows.h>
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

VOID SetIsotropic (HDC hdc, INT cxClient, INT cyClient);
VOID RotatePoint (POINT pt[], INT iNum, INT iAngle);
VOID DrawClock (HDC hdc);
VOID DrawHands (HDC hdc, SYSTEMTIME * pst, BOOL fChange);

#endif /* __CPL_SAMPLE_H */

/* EOF */
