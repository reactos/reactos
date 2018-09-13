#ifdef __cplusplus
extern "C" {
#endif


#define IDM_UPDATE_MACHINE  1
#define IDM_UPDATE_USER     2
#define IDM_PAUSE_MACHINE   3
#define IDM_RESUME_MACHINE  4
#define IDM_PAUSE_USER      5
#define IDM_RESUME_USER     6
#define IDM_EXIT            7
#define IDM_VERBOSE         8
#define IDM_EVENTVWR        9
#define IDM_USERNAME       10
#define IDM_COMPUTERNAME   11
#define IDM_SITENAME       12
#define IDM_SYNCPOLICY     13
#define IDM_DCLIST         14
#define IDM_CHECKGPO       15
#define IDM_CLEARWINDOW    16
#define IDM_SAVEAS         17
#define IDM_PDCNAME        18


#define IDC_NAME           100
#define IDC_DCLIST         101
#define IDC_BROWSE         102
#define IDC_VERSION        103
#define IDC_REGISTRY       104

BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
VOID AddString (LPTSTR lpString);
DWORD NotifyThread (DWORD dwDummy);



BOOL ManageDomainInfo(HWND hWnd);
BOOL CheckGPO(HWND hWnd);


#ifdef __cplusplus
}
#endif
