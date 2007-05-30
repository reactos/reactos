#include "precomp.h"

HINSTANCE hInstance;
HANDLE ProcessHeap;

int WINAPI
_tWinMain(HINSTANCE hThisInstance,
        HINSTANCE hPrevInstance,
        LPTSTR lpCmdLine,
        int nCmdShow)
{
    LPTSTR lpAppName, lpVersion, lpTitle;
    HWND hMainWnd;
    MSG Msg;
    BOOL bRet;
    int Ret = 1;
    size_t len;
    INITCOMMONCONTROLSEX icex;

    hInstance = hThisInstance;
    ProcessHeap = GetProcessHeap();

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&icex);

    if (!AllocAndLoadString(&lpAppName, hInstance, IDS_APPNAME) ||
        !AllocAndLoadString(&lpVersion, hInstance, IDS_VERSION) )
    {
        MessageBox(NULL,
                   _T("Error loading resource "),
                   NULL,
                   0);
        return 1;
    }

    len = _tcslen(lpAppName) + _tcslen(lpVersion);
    lpTitle = (TCHAR*) HeapAlloc(ProcessHeap,
                        0,
                        (len + 2) * sizeof(TCHAR));

    wsprintf(lpTitle,
             _T("%s %s"),
             lpAppName,
             lpVersion);

    if (InitMainWindowImpl())
    {
        if (InitEditWindowImpl())
        {
            hMainWnd = CreateMainWindow(lpTitle,
                                        nCmdShow);
            if (hMainWnd != NULL)
            {
                /* pump the message queue */
                while((bRet = GetMessage(&Msg,
                                         NULL,
                                         0,
                                         0) != 0))
                {
                    if (bRet != (BOOL)-1)
                    {
                        if (!MainWndTranslateMDISysAccel(hMainWnd,
                                                         &Msg))
                        {
                            TranslateMessage(&Msg);
                            DispatchMessage(&Msg);
                        }
                    }
                }

                Ret = 0;
            }

            UninitEditWindowImpl();
        }

        UninitMainWindowImpl();
    }

    LocalFree((HLOCAL)lpAppName);

    return Ret;
}
