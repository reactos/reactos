/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    hardware.h

Abstract:

    Public declarations for the Hardware tab of the System 
    Control Panel Applet

Author:

    William Hsieh (williamh) 03-Jul-1997

Revision History:

    17-Oct-1997 scotthal
        Complete overhaul

--*/
#ifndef _SYSDM_HARDWARE_H_
#define _SYSDM_HARDWARE_H_

//
// Constants and macros
//
#define DEVMGR_FILENAME 	L"devmgr.dll"
#define WIZARD_FILENAME 	L"hdwwiz.cpl"
#define WIZARD_PARAMETERS	L""
#define WIZARD_VERB         L"CPLOpen"
#ifdef UNICODE
#define DEVMGR_EXECUTE_PROC_NAME "DeviceManager_ExecuteW"
#else
#define DEVMGR_EXECUTE_PROC_NAME "DeviceManager_ExecuteA"
#endif

//
// Type definitions
//
typedef BOOL (*PDEVMGR_EXECUTE_PROC)(HWND hwnd, HINSTANCE hInst, LPCTSTR MachineName, int nCmdShow);

HPROPSHEETPAGE 
CreateHardwarePage(
    IN HINSTANCE hInst
);

BOOL 
APIENTRY 
HardwareDlgProc(
    IN HWND hDlg, 
    IN UINT uMsg, 
    IN WPARAM wParam, 
    IN LPARAM lParam
);

#endif // _SYSDM_HARDWARE_H_
