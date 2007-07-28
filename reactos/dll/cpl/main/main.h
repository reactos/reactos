#ifndef __CPL_MAIN_H
#define __CPL_MAIN_H

typedef enum
{
    HWPD_STANDARDLIST = 0,
    HWPD_LARGELIST,
    HWPD_MAX = HWPD_LARGELIST
} HWPAGE_DISPLAYMODE, *PHWPAGE_DISPLAYMODE;

HWND WINAPI
DeviceCreateHardwarePageEx(HWND hWndParent,
                           LPGUID lpGuids,
                           UINT uNumberOfGuids,
                           HWPAGE_DISPLAYMODE DisplayMode);

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
KeyboardApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);

/* main.c */

VOID
InitPropSheetPage(PROPSHEETPAGE *psp,
		  WORD idDlg,
		  DLGPROC DlgProc);


/* mouse.c */

LONG APIENTRY
MouseApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);

#endif /* __CPL_MAIN_H */

/* EOF */
