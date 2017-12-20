
#pragma once

static __inline ATOM RegisterSimpleClass(WNDPROC lpfnWndProc, LPCWSTR lpszClassName)
{
    WNDCLASSEXW wcex;

    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc    = lpfnWndProc;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszClassName  = lpszClassName;
    return RegisterClassExW(&wcex);
}
