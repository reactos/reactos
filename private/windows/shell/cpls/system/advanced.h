/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    advanced.h

Abstract:

    Public declarations for the Advanced tab of the System Control Panel 
    Applet.

Author:

    Scott Hallock (scotthal) 15-Oct-1997

--*/
#ifndef _SYSDM_ADVANCED_H_
#define _SYSDM_ADVANCED_H_

//
// Public function prototypes
//
HPROPSHEETPAGE
CreateAdvancedPage(
    IN HINSTANCE hInst
);


BOOL
APIENTRY
AdvancedDlgProc(
    IN HWND hDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
);

#endif // _SYSDM_ADVANCED_H_
