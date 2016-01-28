/*
 * PROJECT:         ReactOS ODBC Control Panel Applet
 * FILE:            dll/cpl/odbccp32/odbccp32.c
 * PURPOSE:         applet initialization
 * PROGRAMMER:      Johannes Anderwald
 */

#include "odbccp32.h"

HINSTANCE hApplet = NULL;
APPLET_PROC ODBCProc = NULL;
HMODULE hLibrary = NULL;


LONG
CALLBACK
CPlApplet(HWND hwndCpl,
          UINT uMsg,
          LPARAM lParam1,
          LPARAM lParam2)
{
    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_DBLCLK:
        {
            if (ODBCProc == NULL)
            {
                TCHAR szBuffer[MAX_PATH];

                if (ExpandEnvironmentStrings(_T("%systemroot%\\system32\\odbccp32.dll"),
                                             szBuffer,
                                             sizeof(szBuffer) / sizeof(TCHAR)) > 0)
                {
                    hLibrary = LoadLibrary(szBuffer);
                    if (hLibrary)
                    {
                        ODBCProc = (APPLET_PROC)GetProcAddress(hLibrary, "ODBCCPlApplet");
                    }
                }
            }

            if (ODBCProc)
            {
                return ODBCProc(hwndCpl, uMsg, lParam1, lParam2);
            }
            else
            {
                if (hLibrary)
                {
                    FreeLibrary(hLibrary);
                }

                TerminateProcess(GetCurrentProcess(), -1);
                return (LONG)-1;
            }
        }
    }

    return FALSE;
}


BOOL
WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpReserved)
{
    INITCOMMONCONTROLSEX InitControls;
    UNREFERENCED_PARAMETER(lpReserved);

    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        {
            InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
            InitControls.dwICC = ICC_LISTVIEW_CLASSES | ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;
            InitCommonControlsEx(&InitControls);

            hApplet = hinstDLL;
            break;
        }
    }

    return TRUE;
}
