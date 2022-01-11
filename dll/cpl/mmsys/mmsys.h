#ifndef _MMSYS_H
#define _MMSYS_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winuser.h>
#include <mmsystem.h>
#include <cpl.h>
#include <tchar.h>
#include <setupapi.h>
#include <stdlib.h>

#include "resource.h"

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

#define VOLUME_MIN        0
#define VOLUME_MAX      500
#define VOLUME_TICFREQ   50
#define VOLUME_PAGESIZE 100

/* main.c */

VOID
InitPropSheetPage(
    PROPSHEETPAGE *psp,
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

/* speakervolume.c */

INT_PTR
SpeakerVolume(HWND hwndDlg);

#endif /* _MMSYS_H */
