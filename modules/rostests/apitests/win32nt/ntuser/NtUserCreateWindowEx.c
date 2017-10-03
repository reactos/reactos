/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserCreateWindowEx
 */

#include <win32nt.h>

static
inline
HWND
CreateWnd(HINSTANCE hinst,
          PLARGE_STRING clsName,
          PLARGE_STRING clsVer,
          PLARGE_STRING wndName)
{
    return NtUserCreateWindowEx(WS_EX_OVERLAPPEDWINDOW,
                                clsName,
                                clsVer,
                                wndName,
                                WS_CAPTION,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                100,
                                100,
                                NULL,
                                NULL,
                                hinst,
                                0,
                                0,
                                NULL);
}

/* WndProc for class1 */

LRESULT CALLBACK wndProc1(HWND hwnd, UINT msg, WPARAM wPrm, LPARAM lPrm)
{
    return DefWindowProc(hwnd, msg, wPrm, lPrm);
}

/* WndProc for class2 */
LRESULT CALLBACK wndProc2(HWND hwnd, UINT msg, WPARAM wPrm, LPARAM lPrm)
{
    return DefWindowProc(hwnd, msg, wPrm, lPrm);
}


START_TEST(NtUserCreateWindowEx)
{
    HINSTANCE hinst = GetModuleHandle(NULL);
    WNDCLASSEXW wclex = {0};
    WNDCLASSEXW wclex2 = {0};
    WNDCLASSEXW res = {0};

    /* Unicode strings for NtRegisterClassExWOW */
    UNICODE_STRING cls = {14, 32, L"MyClass"};
    UNICODE_STRING ver_cls = {12, 32, L"v2test"};
    UNICODE_STRING another_cls = {10, 32, L"Dummy"};
    UNICODE_STRING menu = {10, 10, L"MuMnu"};
    UNICODE_STRING null_cls = {2, 2, L""};

    /* LARGE_STRING for NtUserCreateWindowEx */
    LARGE_STRING l_dummy = {14, 32, 0, L"DummyMe"};
    LARGE_STRING l_empty = {0, 0, 0, L""};
    LARGE_STRING l_wndName = {32, 32, 0, L""};
    LARGE_STRING l_cls = {cls.Length, 32, 0, cls.Buffer};
    LARGE_STRING l_ver_cls = {ver_cls.Length, 32, 0, ver_cls.Buffer};
    WCHAR bufMe[255] = {0};
    UNICODE_STRING capture = {255, 255, bufMe};
    PWSTR pwstr = NULL;
    CLSMENUNAME clsMenuName, outClsMnu = {0};
    ATOM atom, atom2, atom3;
    HWND hwnd;

    clsMenuName.pszClientAnsiMenuName = "MuMnu";
    clsMenuName.pwszClientUnicodeMenuName = menu.Buffer;
    clsMenuName.pusMenuName = &menu;

    wclex.cbSize = sizeof(WNDCLASSEXW);
    wclex.style = 0;
    wclex.lpfnWndProc = wndProc1;
    wclex.cbClsExtra = 2;
    wclex.cbWndExtra = 4;
    wclex.hInstance = hinst;
    wclex.hIcon = NULL;
    wclex.hCursor = NULL;
    wclex.hbrBackground = CreateSolidBrush(RGB(4,7,5));
    wclex.lpszMenuName = menu.Buffer;
    wclex.lpszClassName = cls.Buffer;
    wclex.hIconSm = NULL;
    memcpy(&wclex2, &wclex, sizeof(wclex));
    wclex2.lpfnWndProc = wndProc2;

    /* Register our first version */
    atom = NtUserRegisterClassExWOW(&wclex,       /* wndClass */
                                    &cls,         /* ClassName */
                                    &cls,         /* Version */
                                    &clsMenuName, /* MenuName */
                                    0,
                                    0,
                                    NULL);
    TEST(atom != 0);

    /* Register second version */
    atom2 = NtUserRegisterClassExWOW(&wclex2,      /* wndClass */
                                     &cls,         /* ClassName */
                                     &ver_cls,     /* Version */
                                     &clsMenuName, /* MenuName */
                                     0,
                                     0,
                                     NULL);

    atom3 = NtUserRegisterClassExWOW(&wclex2,      /* wndClass */
                                    &another_cls, /* ClassName */
                                    &another_cls, /* Version */
                                    &clsMenuName, /* MenuName */
                                    0,
                                    0,
                                    NULL);

    TEST(NtUserRegisterClassExWOW(&wclex2,      /* wndClass */
                                  &cls,         /* ClassName */
                                  NULL,         /* Version */
                                  &clsMenuName, /* MenuName */
                                  0,
                                  0,
                                  NULL) == 0);

    TEST(NtUserRegisterClassExWOW(&wclex2,      /* wndClass */
                                  &cls,         /* ClassName */
                                  &null_cls,    /* Version */
                                  &clsMenuName, /* MenuName */
                                  0,
                                  0,
                                  NULL) == 0);

    TEST(NtUserGetWOWClass(hinst, &ver_cls) != 0);
    TEST(NtUserGetWOWClass(hinst, &ver_cls) != NtUserGetWOWClass(hinst, &cls));    TEST(atom2 != 0);
    TEST(atom == atom2 && (atom | atom2) != 0);

    /* Create a window without versioned class */
    TEST(CreateWnd(hinst, &l_cls, NULL, &l_wndName) == 0);
    TEST(CreateWnd(hinst, &l_cls, &l_wndName, &l_wndName) == 0);

    /* Now, create our first window */
    hwnd = CreateWnd(hinst, &l_cls, &l_cls, &l_wndName);
    TEST(hwnd != 0);
    if(hwnd)
    {
        /* Test some settings about the window */
        TEST((WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC) == wndProc1);

        /* Check class name isn't versioned */
        TEST(NtUserGetClassName(hwnd, TRUE, &capture) != 0);
        TEST(wcscmp(capture.Buffer, cls.Buffer) == 0);
        TEST(wcscmp(capture.Buffer, ver_cls.Buffer) != 0);
        ZeroMemory(capture.Buffer, 255);

        /* Check what return GetClassLong */
        TEST(GetClassLong(hwnd, GCW_ATOM) == atom);
        TEST(NtUserSetClassLong(hwnd, GCW_ATOM, atom3, FALSE) == atom);
        NtUserGetClassName(hwnd, TRUE, &capture);
        TEST(wcscmp(capture.Buffer, another_cls.Buffer) == 0);

        /* Finally destroy it */
        DestroyWindow(hwnd);
    }

    /* Create our second version */
    hwnd = CreateWnd(hinst, &l_cls, &l_ver_cls, &l_wndName);
    TEST(hwnd != 0);
    if (hwnd)
    {
        /* Test settings about window */
        TEST((WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC) == wndProc2);

        /* Check class name isn't versioned */
        TEST(NtUserGetClassName(hwnd, TRUE, &capture) != 0);
        TEST(wcscmp(capture.Buffer, cls.Buffer) == 0);
        TEST(wcscmp(capture.Buffer, ver_cls.Buffer) != 0);
        ZeroMemory(capture.Buffer, 255);

        /* Check what return GetClassLong */
        TEST(GetClassLong(hwnd, GCW_ATOM) == atom);

        TEST(NtUserFindWindowEx(NULL, NULL, &cls, (UNICODE_STRING*)&l_empty, 0) == hwnd);

        /* Finally destroy it */
        DestroyWindow(hwnd);
    }

    /* Create a nonexistent window */
    hwnd = CreateWnd(hinst, &l_cls, &l_dummy, &l_wndName);
    TEST(hwnd == 0);
    if (hwnd) DestroyWindow(hwnd);

    /* Get non-versioned class info */
    res.cbSize = sizeof(res);
    SetLastError(0);
    TEST(NtUserGetClassInfo(hinst, &cls, &res, &pwstr, 0) != 0);
    TEST(GetLastError() == 0);
    TEST(res.cbSize == wclex.cbSize);
    TEST(res.style == wclex.style);
    TEST(res.lpfnWndProc == wclex.lpfnWndProc);
    TEST(res.cbClsExtra == wclex.cbClsExtra);
    TEST(res.cbWndExtra == wclex.cbWndExtra);
    TEST(res.hInstance == wclex.hInstance);
    TEST(res.hIcon == wclex.hIcon);
    TEST(res.hCursor == wclex.hCursor);
    TEST(res.hbrBackground == wclex.hbrBackground);
    TEST(res.lpszMenuName == 0);
    TEST(res.lpszClassName == 0);
    TEST(res.hIconSm == wclex.hIconSm);

    /* Get versioned class info */
    TEST(NtUserGetClassInfo(hinst, &ver_cls, &res, &pwstr, 0) == atom2);
    TEST(GetLastError() == 0);
    TEST(res.cbSize == wclex2.cbSize);
    TEST(res.style == wclex2.style);
    TEST(res.lpfnWndProc == wclex2.lpfnWndProc);
    TEST(res.cbClsExtra == wclex2.cbClsExtra);
    TEST(res.cbWndExtra == wclex2.cbWndExtra);
    TEST(res.hInstance == wclex2.hInstance);
    TEST(res.hIcon == wclex2.hIcon);
    TEST(res.hCursor == wclex2.hCursor);
    TEST(res.hbrBackground == wclex2.hbrBackground);
    TEST(res.lpszMenuName == 0);
    TEST(res.lpszClassName == 0);
    TEST(res.hIconSm == wclex2.hIconSm);

    /* Create a new window from our old class. Since we set a new class atom,
     * it should be set to our new atom
     */
    hwnd = NULL;
    hwnd = CreateWnd(hinst, &l_cls, &l_cls, &l_wndName);
    TEST(hwnd != NULL);
    if (hwnd)
    {
        TEST(GetClassLong(hwnd, GCW_ATOM) == atom3);
        TEST(NtUserGetClassName(hwnd, TRUE, &capture) != 0);
        TEST(wcscmp(capture.Buffer, another_cls.Buffer) == 0);
        DestroyWindow(hwnd);
    }

    /* Test class destruction */
    TEST(NtUserUnregisterClass(&cls, hinst, (PCLSMENUNAME)0xbad) != 0);
    TEST(NtUserUnregisterClass(&ver_cls, hinst, &outClsMnu) != 0);
    TEST(NtUserUnregisterClass(&another_cls, hinst, &outClsMnu) != 0);
    TEST(NtUserUnregisterClass(&menu, hinst, &outClsMnu) == 0);

    /* Make sure that the classes got destroyed */
    TEST(NtUserGetWOWClass(hinst, &cls) == 0);
    TEST(NtUserGetWOWClass(hinst, &ver_cls) == 0);
    TEST(NtUserGetWOWClass(hinst, &another_cls) == 0);
}
