/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    hwprof.h

Abstract:

    Public declarations for the Hardware Profiles dialog.

Author:

    Paula Tomlinson (paulat) 8-22-1995

Revision History:

    22-Aug-1995     paulat
        Creation and initial implementation.

    17-Oc-1997 scotthal
        Split public declarations into their own header file

--*/
#ifndef _SYSDM_HWPROF_H_
#define _SYSDM_HWPROF_H_

//
// Public function prototypes
//
BOOL 
APIENTRY 
HardwareProfilesDlg(
    IN HWND hDlg, 
    IN UINT uMessage, 
    IN WPARAM wParam, 
    IN LPARAM lParam
);

BOOL 
APIENTRY 
CopyProfileDlg(
    IN HWND hDlg, 
    IN UINT uMessage, 
    IN WPARAM wParam, 
    IN LPARAM lParam
);

BOOL 
APIENTRY 
RenameProfileDlg(
    IN HWND hDlg, 
    IN UINT uMessage, 
    IN WPARAM wParam, 
    IN LPARAM lParam
);

BOOL 
APIENTRY GeneralProfileDlg(
    IN HWND hDlg, 
    IN UINT uMessage, 
    IN WPARAM wParam, 
    IN LPARAM lParam
);

#endif // _SYSDM_HWPROF_H_
