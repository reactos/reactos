#ifndef __CPL_SYSDM_H
#define __CPL_SYSDM_H

typedef LONG (CALLBACK *APPLET_PROC)(VOID);

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

void ShowLastWin32Error(HWND hWndOwner);

#endif /* __CPL_SYSDM_H */

/* EOF */
