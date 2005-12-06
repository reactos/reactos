#ifndef __CPL_APPWIZ_H
#define __CPL_APPWIZ_H

typedef LONG (CALLBACK *CPLAPPLET_PROC)(VOID);

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  CPLAPPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

void ShowLastWin32Error(HWND hWndOwner);

#endif /* __CPL_APPWIZ_H */

/* EOF */
