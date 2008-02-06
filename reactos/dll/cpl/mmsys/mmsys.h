#ifndef __CPL_MMSYS_H
#define __CPL_MMSYS_H

//typedef LONG (CALLBACK *APPLET_PROC)(VOID);

typedef struct _APPLET
{
  UINT idIcon;
  UINT idName;
  UINT idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;


#define DRVM_MAPPER 0x2000
#define DRVM_MAPPER_PREFERRED_GET (DRVM_MAPPER+21)
#define DRVM_MAPPER_PREFERRED_SET (DRVM_MAPPER+22)

/* main.c */

VOID
InitPropSheetPage(PROPSHEETPAGE *psp,
		  WORD idDlg,
		  DLGPROC DlgProc);

LONG APIENTRY
MmSysApplet(HWND hwnd,
            UINT uMsg,
            LPARAM wParam,
            LPARAM lParam);

/* sounds.c */

INT_PTR
CALLBACK
SoundsDlgProc(HWND hwndDlg,
	        UINT uMsg,
	        WPARAM wParam,
	        LPARAM lParam);

/* volume.c */

INT_PTR CALLBACK
VolumeDlgProc(HWND hwndDlg,
	        UINT uMsg,
	        WPARAM wParam,
	        LPARAM lParam);

/* voice.c */

INT_PTR CALLBACK
VoiceDlgProc(HWND hwndDlg,
             UINT uMsg,
             WPARAM wParam,
             LPARAM lParam);

/* audio.c */

INT_PTR CALLBACK
AudioDlgProc(HWND hwndDlg,
             UINT uMsg,
             WPARAM wParam,
             LPARAM lParam);

#endif /* __CPL_MMSYS_H */

/* EOF */
