
#include "xboxogl.h"

BOOL WINAPI
DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    (void)inst; (void)reserved;
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(inst);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
