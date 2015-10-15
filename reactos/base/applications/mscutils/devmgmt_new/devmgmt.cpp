#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cfgmgr32.h>
#include <devmgr/devmgr.h>

int WINAPI
wWinMain(HINSTANCE hThisInstance,
         HINSTANCE hPrevInstance,
         LPWSTR lpCmdLine,
         int nCmdShow)
{
    if (!DeviceManager_ExecuteW(NULL,
                                hThisInstance,
                                NULL,
                                nCmdShow))
    {
        return GetLastError();
    }
    return 0;
}
