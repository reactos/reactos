#ifndef __dll_h
#define __dll_h

extern HINSTANCE g_hInstance; 
#define GLOBAL_HINSTANCE (g_hInstance)

STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved);
STDAPI DllCanUnloadNow(VOID);
STDAPI DllGetClassObject(REFCLSID rCLSID, REFIID riid, LPVOID* ppvVoid);
STDAPI DllInstall(BOOL fInstall, LPTSTR pCmdLine);

#endif
