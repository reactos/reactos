//*************************************************************
//  File name:    PROFILE.H
//
//  Description:  Header file for profile control panel applet
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1992-1994
//  All rights reserved
//
//*************************************************************

//
// Misc. constants
//

#define NUM_APPLETS 1
#define MAX_DOMAIN_NAME             128
#define MAX_USER_NAME               128
#define MAX_COMPUTER_NAME           128
#define MAX_TEMP_BUFFER            (128 + MAX_DOMAIN_NAME + MAX_USER_NAME)
#define MAX_NUM_COMPUTERS           256
#define PROFILE_NAME_LEN             30
#define MAX_ERROR_MSG               350
#define NAMES_HELP_CONTEXT            1
#define UNKNOWN_LEN                  30

#define LOCAL_PROFILE_TYPE          TEXT('0')
#define PERSONAL_PROFILE_TYPE       TEXT('1')
#define MANDITORY_PROFILE_TYPE      TEXT('2')


//
// Icon ID
//

#define ID_ICON 1

//
// String table constants
//

#define IDS_NAME                      1
#define IDS_INFO                      2
#define IDS_UNABLETOSAVE              3
#define IDS_BASEERRORMSG              4
#define IDS_NAMEINDONTLIST            5
#define IDS_NONAMEANDDONOTSAVE        6
#define IDS_LOGOFFNOTICE              7
#define IDS_DUPLICATENAME             8
#define IDS_ADDNAME                   9
#define IDS_UNKNOWN                  10
#define IDS_FORMAT                   11

//
// Dialog box constants
//
#define IDD_PROFILE                 100
#define IDD_USERNAME                101
#define IDD_PATH                    102
#define IDD_DONTSAVECHANGE          103
#define IDD_SAVELIST                104
#define IDD_DEFAULTSAVE             105
#define IDD_DEFAULTDONTSAVE         106
#define IDD_HELP                    107
#define IDD_SAVECHANGE              108
#define IDD_DONTSAVELIST            109
#define IDD_COMPUTERNAME            110

#define IDD_COMPUTERNAMES           200
#define IDD_DELETE                  201
#define IDD_CLEARALL                202
#define IDD_NAMESHELP               203
#define IDD_NAMESLIST               204
#define IDD_NEWNAME                 205
#define IDD_ADD                     206

//
// Global Variables
//

extern HINSTANCE hInstance;
extern LPTSTR    glpList;
extern TCHAR     szProfileRegInfo[];
extern TCHAR     szProfileType[];
extern UINT      uiShellHelp;
extern TCHAR     szShellHelp[];
extern TCHAR     szHelpFileName[];


//
// Function prototypes
//

void   RunApplet(HWND);
BOOL   InitializeDialog (HWND);
void   ParseAndAddComputerNames(HWND, WORD, LPTSTR);
LPTSTR CreateList (HWND, WORD, LPTSTR, LPBOOL);
BOOL   SaveSettings (HWND);
BOOL   CompareLists (HWND, WORD, WORD);
BOOL   CheckProfileType (void);
void   SetDefButton(HWND, INT);

LRESULT CALLBACK ProfileDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK NamesDlgProc(HWND, UINT, WPARAM, LPARAM);


//
// Macros
//

#if DBG

#define KdPrint(_x_) \
         OutputDebugStringA ("PROFILE:  "); \
         OutputDebugStringA _x_; \
         OutputDebugStringA ("\r\n");

#else

#define KdPrint(_x_)

#endif
