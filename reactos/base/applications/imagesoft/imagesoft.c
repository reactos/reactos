#include <precomp.h>

HINSTANCE hInstance;
HANDLE ProcessHeap;

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif
int WINAPI
WinMain(HINSTANCE hThisInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
    LPTSTR lpAppName;
    HWND hMainWnd;
    MSG Msg;
    BOOL bRet;
    int Ret = 1;
    INITCOMMONCONTROLSEX icex;

    hInstance = hThisInstance;
    ProcessHeap = GetProcessHeap();

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&icex);

    if (!AllocAndLoadString(&lpAppName,
                            hInstance,
                            IDS_APPNAME))
    {
        return 1;
    }

    if (TbdInitImpl())
    {
        if (InitMainWindowImpl())
        {
            if (InitImageEditWindowImpl())
            {
                hMainWnd = CreateMainWindow(lpAppName,
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

                UninitImageEditWindowImpl();
            }

            UninitMainWindowImpl();
        }

        TbdUninitImpl();
    }

    LocalFree((HLOCAL)lpAppName);

    return Ret;
}
