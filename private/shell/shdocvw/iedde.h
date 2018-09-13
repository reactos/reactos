#ifndef _IEDDE_H
#define _IEDDE_H

BOOL IEDDE_Initialize(void);
void IEDDE_Uninitialize(void);
void IEDDE_AutomationStarted(void);
HRESULT IEDDE_BeforeNavigate(LPCWSTR pwszURL, BOOL *pfCanceled);
HRESULT IEDDE_AfterNavigate(LPCWSTR pwszURL, HWND hwnd);
BOOL IEDDE_NewWindow(HWND hwnd);
BOOL IEDDE_WindowDestroyed(HWND hwnd);

#endif  //_IEDDE_H
