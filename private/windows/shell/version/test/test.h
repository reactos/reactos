#define IDC_DUMMY                   -1
#define IDD_ABOUT	100
#define IDD_FIND	101
#define IDD_INSTALL	102
#define IDD_INFO	103
#define IDD_QUERY	104
#define IDD_LANG	105

#define IDC_FILENAME    201
#define IDC_SUBLANGID   202
#define IDC_LANGID      203

#define IDM_VERSION	300

#define IDM_ABOUT	301
#define IDM_EXIT	302
#define IDM_FIND	303
#define IDM_INSTALL	304
#define IDM_INFO	305
#define IDM_QUERY	306
#define IDM_LANG	307
#define IDM_FREE	308

BOOL	InitApplication(HANDLE);
BOOL	InitInstance(HANDLE, int);
long	MainWndProc(HWND, UINT, UINT, LONG);
BOOL	About(HWND, UINT, UINT, LONG);
BOOL	Find(HWND, UINT, UINT, LONG);
BOOL	Install(HWND, UINT, UINT, LONG);
BOOL	Information(HWND, UINT, UINT, LONG);
BOOL	Query(HWND, UINT, UINT, LONG);
BOOL	Language(HWND, UINT, UINT, LONG);
