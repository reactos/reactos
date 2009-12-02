#ifndef POWERCFG_H
#define POWERCFG_H

#include "powrprof.h"

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;
extern GLOBAL_POWER_POLICY gGPP;
extern POWER_POLICY gPP[];
extern UINT guiIndex;

#define MAX_POWER_PAGES 32

INT_PTR CALLBACK PowerSchemesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AlarmsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AdvancedDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HibernateDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif /* __CPL_SAMPLE_H */

/* EOF */
