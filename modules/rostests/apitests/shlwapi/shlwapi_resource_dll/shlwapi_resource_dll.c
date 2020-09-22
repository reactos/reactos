#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windows.h>

BOOL WINAPI
DllMain(IN HINSTANCE hDllHandle,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        HANDLE hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, "Local\\shlwapi_apitest_evt");
        if (hEvent != NULL)
        {
            SetEvent(hEvent);
            CloseHandle(hEvent);
        }
        else
        {
            OutputDebugStringA("ERROR: NO EVENT FOUND!\n");
        }
    }

    return TRUE;
}
