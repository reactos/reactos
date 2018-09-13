/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    edtenvar.h

Abstract:

    Public declarations for the Edit Environment Variables dialog of the
    System Control Panel Applet
    
Author:

    Scott Hallock (scotthal) 11-Nov-1997

Revision History:


--*/

//
// Preprocessor definitions
//
#define SYSTEM_VAR        1
#define USER_VAR          2
#define INVALID_VAR_TYPE  0xeeee

#define EDIT_VAR          1
#define NEW_VAR           2
#define INVALID_EDIT_TYPE 0xeeee

#define EDIT_NO_CHANGE    0
#define EDIT_CHANGE       1
#define EDIT_ERROR       (-1)

#define EDIT_ENVVAR_CAPTION_LENGTH 128

//
// Global variables
//
extern UINT g_VarType;
extern UINT g_EditType;
extern TCHAR g_szVarName[BUFZ];
extern TCHAR g_szVarValue[BUFZ];

//
// Function prototypes
//
INT_PTR
APIENTRY
EnvVarsEditDlg(
    IN HWND hDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
);
