#ifndef __dll_h
#define __dll_h

extern HINSTANCE g_hInstance;
#define GLOBAL_HINSTANCE (g_hInstance)

EXTERN_C BOOL DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved );
STDAPI DllCanUnloadNow( void );
STDAPI DllGetClassObject( REFCLSID rCLSID, REFIID riid, LPVOID* ppvVoid );

#define T_HKEY   0x0001
#define T_VALUE  0x0002
#define T_DWORD  0x0003
#define T_END    0xFFFF

HRESULT CallRegInstall(HINSTANCE hInstance, LPSTR szSection);

#ifdef DEBUG
void DllSetTraceMask(void);
#endif


#endif
