/* $Id$
 */

#include <windows.h>
//#include <ddraw.h>

HRESULT WINAPI DirectSoundCreate(
  LPGUID lpGuid, 
  DWORD* ppDS, 
  LPUNKNOWN  pUnkOuter 
)
{
    return E_FAIL;
}

BOOL WINAPI DllMain(HINSTANCE hInstance,DWORD fwdReason, LPVOID lpvReserved)
{
    switch(fwdReason)
    {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_PROCESS_DETACH:
            break;
        case DLL_THREAD_DETACH:
            break;
    }
    return(TRUE);
} 
