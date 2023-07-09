#define IDM_CPD              100
#define IDM_EXIT             101
#define IDM_ABOUT            102
#define IDM_LUP              103
#define IDM_ULUP             104
#define IDM_LOGON            105
#define IDM_LOGOFF           106
#define IDM_PFTYPE           107

#define IDD_PROFILEPATH      601
#define IDD_KEYNAMEBOX       602
#define IDD_KEYNAME          603
#define IDD_TIME             605
#define IDD_PROFILE          606
#define IDD_TIME_TEXT        607
#define IDD_PROFILE_TEXT     608
#define IDD_DEFAULTPATH      609
#define IDD_RETVAL           610
#define IDD_NOUI             611
#define IDD_APPLYPOLICY      612
#define IDD_LITELOAD         613

#define IDD_USERNAME         700
#define IDD_DOMAIN           701
#define IDD_PASSWORD         702
#define IDD_ICON             703

BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About  (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LUPDlgProc  (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LogonDlgProc(HWND, UINT, WPARAM, LPARAM);
