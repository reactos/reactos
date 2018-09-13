/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    perf.h

Abstract:

    Public declarations for the Performance dialog of the 
    System Control Panel Applet

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#ifndef _SYSDM_PERF_H_
#define _SYSDM_PERF_H_

//
//  Reboot switch for crashdump dlg
//

#define RET_ERROR               (-1)
#define RET_NO_CHANGE           0x00
#define RET_VIRTUAL_CHANGE      0x01
#define RET_RECOVER_CHANGE      0x02
#define RET_CHANGE_NO_REBOOT    0x04
#define RET_CONTINUE            0x08
#define RET_BREAK               0x10

#define RET_VIRT_AND_RECOVER (RET_VIRTUAL_CHANGE | RET_RECOVER_CHANGE)



//
// Public function declarations
//
HPROPSHEETPAGE 
CreatePerformancePage(
    IN HINSTANCE hInst
);

BOOL 
APIENTRY 
PerformanceDlgProc(
    IN HWND hDlg, 
    IN UINT uMsg, 
    IN WPARAM wParam, 
    IN LPARAM lParam
);

#endif // _SYSDM_PERF_H_
