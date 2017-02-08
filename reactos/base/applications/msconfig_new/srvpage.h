#ifndef _SRVPAGE_H_
#define _SRVPAGE_H_

DWORD GetServicesActivation(VOID);
INT_PTR CALLBACK ServicesPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif
