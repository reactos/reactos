//
//  The Shutdown Query dialog and Logoff Windows NT dialog
//  are shared by Progman (included in windows\shell\progman\progman.dlg),
//  and therefore changes to them or the filename should not be made
//  unless tested with Progman first.
//  This header file is included in windows\shell\progman\pmdlg.h
//
//  11/10/92  johannec
//

#define IDD_SHUTDOWN_QUERY          1200
#define IDD_CLOSEAPPS               1201
#define IDD_RESTART                 1202
#define IDD_END_WINDOWS_SESSION     1203
#define IDD_POWEROFF                1204

#define DLGSEL_LOGOFF                  0
#define DLGSEL_SHUTDOWN                1
#define DLGSEL_SHUTDOWN_AND_RESTART    2
#define DLGSEL_SHUTDOWN_AND_POWEROFF   3

#define WINLOGON_KEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
#define SHUTDOWN_SETTING_KEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Shutdown"
#define SHUTDOWN_SETTING L"Shutdown Setting"

