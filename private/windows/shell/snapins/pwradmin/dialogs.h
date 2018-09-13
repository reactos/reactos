
VOID DisplayPropertyPage(long Item);
#define ITEM_SYSTEM 1
#define ITEM_MONITOR 2

INT_PTR CALLBACK SystemDlgProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK MonitorDlgProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

