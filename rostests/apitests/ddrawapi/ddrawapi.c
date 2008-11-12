#include "ddrawapi.h"

HINSTANCE g_hInstance;

BOOL
IsFunctionPresent(LPWSTR lpszFunction)
{
    return TRUE;
}

int APIENTRY
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR     lpCmdLine,
        int       nCmdShow)
{
    g_hInstance = hInstance;
    return TestMain(L"ddrawapi", L"ddraw.dll");
}


