#include <precomp.h>

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

    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&icex);

    if ( !AllocAndLoadString(&lpAppName, hInstance, IDS_APPNAME) ||
         !AllocAndLoadString(&lpVersion, hInstance, IDS_VERSION) )
    {
        return Ret;
    }

    len = _tcslen(lpAppName) + _tcslen(lpVersion);
    lpTitle = HeapAlloc(ProcessHeap,
                        0,
                        (len + 2) * sizeof(TCHAR));
    if (lpTitle == NULL)
    {
        LocalFree((HLOCAL)lpAppName);
        LocalFree((HLOCAL)lpVersion);
        return Ret;
    }

    wsprintf(lpTitle,
             _T("%s %s"),
             lpAppName,
             lpVersion);

    LocalFree((HLOCAL)lpAppName);
    LocalFree((HLOCAL)lpVersion);

    if (TbdInitImpl())
    {
        if (InitMainWindowImpl())
        {
            if (InitImageEditWindowImpl())
            {
                if (InitFloatWndClass())
                {
                    hMainWnd = CreateMainWindow(lpTitle,
                                                nCmdShow);
                    if (hMainWnd != NULL)
                    {
                        /* pump the message queue */
                        while ((bRet = GetMessage(&Msg,
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

                    UninitImageEditWindowImpl();
                }

                UninitFloatWndImpl();
            }

            UninitMainWindowImpl();
        }

        TbdUninitImpl();
    }

    HeapFree(GetProcessHeap(),
             0,
             lpTitle);

    return Ret;
}
