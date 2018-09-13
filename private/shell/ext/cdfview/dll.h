//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// dll.h 
//
//   Definitions for dll.cpp.
//
//   History:
//
//       3/16/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _DLL_H_

#define _DLL_H_

//
// Function prototypes.
//

EXTERN_C BOOL WINAPI DllMain(HANDLE hInst, DWORD dwReason, LPVOID pReserved);

EXTERN_C STDAPI DllCanUnloadNow(void);
EXTERN_C STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppvObj);
EXTERN_C STDAPI DllRegisterServer(void);
EXTERN_C STDAPI DllUnregisterServer(void);

HRESULT RegisterServerHelper(LPSTR szCmd);
void DllAddRef(void);
void DllRelease(void);

#define PRELOAD_MSXML       0x0001
#define PRELOAD_WEBCHECK    0x0002

void DLL_ForcePreloadDlls(DWORD dwFlags);

#endif _DLL_H_
