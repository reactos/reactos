#ifndef _SYSSETUP_PCH_
#define _SYSSETUP_PCH_

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winuser.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <setupapi.h>
#include <syssetup/syssetup.h>
#include <pseh/pseh2.h>
#include <cfgmgr32.h>

#include "globals.h"
#include "resource.h"

#define EnableDlgItem(hDlg, nID, bEnable)   \
    EnableWindow(GetDlgItem((hDlg), (nID)), (bEnable))

#define ShowDlgItem(hDlg, nID, nCmdShow)    \
    ShowWindow(GetDlgItem((hDlg), (nID)), (nCmdShow))

/* These are public names and values determined from MFC, and compatible with Windows */
// Property Sheet control IDs (determined with Spy++)
#define IDC_TAB_CONTROL                 0x3020
#define ID_APPLY_NOW                    0x3021
#define ID_WIZBACK                      0x3023
#define ID_WIZNEXT                      0x3024
#define ID_WIZFINISH                    0x3025

#endif /* _SYSSETUP_PCH_ */
