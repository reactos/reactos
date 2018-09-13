/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    envvar.h

Abstract:

    Public declarations for the Environment Variables dialog of the 
    System Control Panel Applet

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#ifndef _SYSDM_ENVVAR_H_
#define _SYSDM_ENVVAR_H_

#define MAX_USER_NAME   100

#define BUFZ              4096
#define MAX_VALUE_LEN     1024

//  Environment variables structure
typedef struct
{
    DWORD  dwType;
    LPTSTR szValueName;
    LPTSTR szValue;
    LPTSTR szExpValue;
} ENVARS, *PENVAR;

HPROPSHEETPAGE 
CreateEnvVarsPage(
    IN HINSTANCE hInst
);

INT_PTR 
APIENTRY 
EnvVarsDlgProc(
    IN HWND hDlg, 
    IN UINT uMsg, 
    IN WPARAM wParam, 
    IN LPARAM lParam
);



#endif // _SYSDM_ENVVAR_H_
