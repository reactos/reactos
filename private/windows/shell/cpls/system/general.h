/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    general.h

Abstract:

    Public declarations for the General tab of the System Control Panel
    Applet

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#ifndef _SYSDM_GENERAL_H_
#define _SYSDM_GENERAL_H_

HPROPSHEETPAGE 
CreateGeneralPage(
    IN HINSTANCE hInst
);

BOOL 
APIENTRY GeneralDlgProc(
    IN HWND hDlg, 
    IN UINT uMsg, 
    IN WPARAM wParam, 
    IN LPARAM lParam
);

#endif // _SYSDM_GENERAL_H_
