#ifndef __CPL_SYSDM_H
#define __CPL_SYSDM_H

typedef LONG (CALLBACK *APPLET_INITPROC)(VOID);

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  APPLET_INITPROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

void ShowLastWin32Error(HWND hWndOwner);

#endif /* __CPL_SYSDM_H */

/* EOF */
