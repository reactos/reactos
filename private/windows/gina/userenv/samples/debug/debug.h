//
// Debug levels
//

#define DL_NONE     0x00000000
#define DL_NORMAL   0x00000001
#define DL_VERBOSE  0x00000002
#define DL_LOGFILE  0x00010000
#define DL_DEBUGGER 0x00020000

//
// Winlogon location
//

#define WINLOGON_KEY        TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define EXEC_OPTIONS_KEY    TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\Winlogon.exe")

#define USERENV_DEBUG_LEVEL TEXT("UserEnvDebugLevel")
#define GLOBAL_FLAG         TEXT("GlobalFlag")

#define IDD_NONE           401
#define IDD_NORMAL         402
#define IDD_VERBOSE        403
#define IDD_LOGFILE        404
#define IDD_WINLOGON       405

BOOL CALLBACK DebugDlgProc (HWND, UINT, WPARAM, LPARAM);
