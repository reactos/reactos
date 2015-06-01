#include "stdafx.h"
#include "devmgmt.h"
#include "MainWindow.h"

HINSTANCE g_hInstance = NULL;
HANDLE ProcessHeap = NULL;

int WINAPI
wWinMain(HINSTANCE hThisInstance,
         HINSTANCE hPrevInstance,
         LPWSTR lpCmdLine,
         int nCmdShow)
{
    CMainWindow MainWindow;
    INITCOMMONCONTROLSEX icex;
    HANDLE hMutex;
    CAtlString szAppName;

    int Ret = 1;

    /* Check if the app is already running */
    hMutex = CreateMutexW(NULL, TRUE, L"devmgmt_mutex2");
    if (hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        /* Cleanup and exit */
        //if (hMutex) CloseHandle(hMutex);
        //return 0;
    }

    /* Store the global values */
    g_hInstance = hThisInstance;
    ProcessHeap = GetProcessHeap();

    /* Initialize common controls */
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&icex);

    /* Load the application name */
    if (szAppName.LoadStringW(g_hInstance, IDS_APPNAME))
    {
        /* Initialize the main window */
        if (MainWindow.Initialize(szAppName, nCmdShow))
        {
            /* Run the application */
            Ret = MainWindow.Run();

            /* Uninitialize the main window */
            MainWindow.Uninitialize();
        }
    }

    /* Delete the app mutex */
    CloseHandle(hMutex);

    return Ret;
}
