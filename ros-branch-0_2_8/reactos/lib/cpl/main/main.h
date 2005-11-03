#ifndef __CPL_MAIN_H
#define __CPL_MAIN_H

//typedef LONG (CALLBACK *APPLET_PROC)(VOID);

typedef struct _APPLET
{
  UINT idIcon;
  UINT idName;
  UINT idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;


/* keyboard.c */

LONG APIENTRY
KeyboardApplet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam);

/* main.c */

VOID
InitPropSheetPage(PROPSHEETPAGE *psp,
		  WORD idDlg,
		  DLGPROC DlgProc);


/* mouse.c */

LONG APIENTRY
MouseApplet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam);

#endif /* __CPL_MAIN_H */

/* EOF */
