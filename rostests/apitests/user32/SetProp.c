/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for SetProp
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#include <winuser.h>
#include "helper.h"

static ATOM Atom1, Atom2, Atom3;

static
BOOL
CALLBACK
EnumFunc(
    _In_ HWND hWnd,
    _In_ PCWSTR lpszString,
    _In_ HANDLE hData)
{
    if (HIWORD(lpszString))
        ok(0, "Unexpected EnumFunc call: %p, '%ls', %p\n", hWnd, lpszString, hData);
    else
        ok(0, "Unexpected EnumFunc call: %p, 0x%04x, %p\n", hWnd, (USHORT)(ULONG_PTR)lpszString, hData);
    return TRUE;
}

static
BOOL
CALLBACK
EnumFuncEx(
    _In_ HWND hWnd,
    _In_ PWSTR lpszString,
    _In_ HANDLE hData,
    _In_ ULONG_PTR dwData)
{
    if (dwData == 0)
    {
        if (HIWORD(lpszString))
            ok(0, "Unexpected EnumFuncEx call: %p, '%ls', %p\n", hWnd, lpszString, hData);
        else
            ok(0, "Unexpected EnumFuncEx call: %p, 0x%04x, %p\n", hWnd, (USHORT)(ULONG_PTR)lpszString, hData);
    }
    else
    {
        if (HIWORD(lpszString))
        {
            if (!wcscmp(lpszString, L"PropTestAtom1"))
                ok(hData == &Atom1, "EnumFuncEx: %p, '%ls', %p; expected %p\n", hWnd, lpszString, hData, &Atom1);
            else if (!wcscmp(lpszString, L"PropTestAtom2"))
                ok(hData == &Atom2, "EnumFuncEx: %p, '%ls', %p; expected %p\n", hWnd, lpszString, hData, &Atom2);
            else
                ok(0, "Unexpected EnumFuncEx call: %p, '%ls', %p\n", hWnd, lpszString, hData);
        }
        else
            ok(0, "Unexpected EnumFuncEx call: %p, 0x%04x, %p\n", hWnd, (USHORT)(ULONG_PTR)lpszString, hData);
    }
    return TRUE;
}

START_TEST(SetProp)
{
    HWND hWnd;
    MSG msg;
    UINT Atom;
    HANDLE Prop;
    LRESULT Result;
    ATOM SysICAtom;
    ATOM SysICSAtom;
    HICON hIcon;
    HICON hIcon2;

    Atom1 = GlobalAddAtomW(L"PropTestAtom1");
    ok(Atom1 != 0, "PropTestAtom1 is 0x%04x\n", Atom1);
    ok(Atom1 >= 0xc000, "PropTestAtom1 is 0x%04x\n", Atom1);
    ok(Atom1 >= 0xc018, "PropTestAtom1 is 0x%04x\n", Atom1);

    /* These are not in the global atom table */
    Atom = GlobalFindAtomW(L"SysIC");
    ok(Atom == 0, "SysIC atom is 0x%04x\n", Atom);
    Atom = GlobalFindAtomW(L"SysICS");
    ok(Atom == 0, "SysICS atom is 0x%04x\n", Atom);

    SetCursorPos(0, 0);

    RegisterSimpleClass(DefWindowProcW, L"PropTest");

    hWnd = CreateWindowExW(0, L"PropTest", NULL, 0,  10, 10, 20, 20,  NULL, NULL, 0, NULL);

    Result = EnumPropsW(hWnd, EnumFunc);
    if (0) // Windows returns an uninitialized value here
    ok(Result == TRUE, "EnumProps returned %Iu\n", Result);
    Result = EnumPropsExW(hWnd, EnumFuncEx, 0);
    if (0) // Windows returns an uninitialized value here
    ok(Result == TRUE, "EnumPropsEx returned %Iu\n", Result);

    Atom2 = GlobalAddAtomW(L"PropTestAtom2");
    ok(Atom2 != 0, "PropTestAtom2 is 0x%04x\n", Atom2);
    ok(Atom2 >= 0xc000, "PropTestAtom2 is 0x%04x\n", Atom2);
    ok(Atom2 >= 0xc018, "PropTestAtom2 is 0x%04x\n", Atom2);

    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        DispatchMessageA(&msg);

    Result = EnumPropsExW(hWnd, EnumFuncEx, 0);
    if (0) // Windows returns an uninitialized value here
    ok(Result == TRUE, "EnumPropsEx returned %Iu\n", Result);

    Result = SetPropW(hWnd, (PCWSTR)MAKEINTATOM(Atom1), &Atom1);
    ok(Result == TRUE, "SetProp returned %Iu\n", Result);
    Result = SetPropW(hWnd, (PCWSTR)MAKEINTATOM(Atom2), &Atom3);
    ok(Result == TRUE, "SetProp returned %Iu\n", Result);
    Prop = GetPropW(hWnd, (PCWSTR)MAKEINTATOM(Atom2));
    ok(Prop == &Atom3, "GetProp returned %p, expected %p\n", Prop, &Atom3);
    Result = SetPropW(hWnd, (PCWSTR)MAKEINTATOM(Atom2), &Atom2);
    ok(Result == TRUE, "SetProp returned %Iu\n", Result);
    Prop = GetPropW(hWnd, (PCWSTR)MAKEINTATOM(Atom2));
    ok(Prop == &Atom2, "GetProp returned %p, expected %p\n", Prop, &Atom2);
    Prop = GetPropW(hWnd, L"PropTestAtom2");
    ok(Prop == &Atom2, "GetProp returned %p, expected %p\n", Prop, &Atom2);
    Prop = GetPropA(hWnd, "PropTestAtom2");
    ok(Prop == &Atom2, "GetProp returned %p, expected %p\n", Prop, &Atom2);

    Result = EnumPropsExW(hWnd, EnumFuncEx, 1);
    ok(Result == TRUE, "EnumPropsEx returned %Iu\n", Result);

    hIcon = LoadImageW(NULL, (PCWSTR)MAKEINTRESOURCE(OIC_NOTE), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    ok(hIcon != NULL, "LoadImage failed with %lu\n", GetLastError());
    /* Should not have any icon */
    hIcon2 = (HICON)SendMessageW(hWnd, WM_GETICON, ICON_BIG, 0);
    ok(hIcon2 == NULL, "WM_GETICON returned %p, expected NULL\n", hIcon2);
    hIcon2 = (HICON)SendMessageW(hWnd, WM_GETICON, ICON_SMALL, 0);
    ok(hIcon2 == NULL, "WM_GETICON returned %p, expected NULL\n", hIcon2);

    /* Set big icon, should also set small icon */
    Result = SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    ok(Result == 0, "WM_SETICON returned 0x%Ix\n", Result);

    hIcon2 = (HICON)SendMessageW(hWnd, WM_GETICON, ICON_BIG, 0);
    ok(hIcon2 == hIcon, "WM_GETICON returned %p, expected %p\n", hIcon2, hIcon);
    hIcon2 = (HICON)SendMessageW(hWnd, WM_GETICON, ICON_SMALL, 0);
    ok(hIcon2 != hIcon && hIcon != NULL, "WM_GETICON returned %p, expected not %p and not NULL\n", hIcon2, hIcon);

    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        DispatchMessageA(&msg);

    /* We should have only the props that we explicitly set */
    for (Atom = 0x0000; Atom <= 0xffff; Atom++)
    {
        Prop = GetPropW(hWnd, (PCWSTR)MAKEINTATOM(Atom));
        if (Atom == Atom1)
            ok(Prop == &Atom1, "Window %p Prop 0x%04x = %p, expected %p\n", hWnd, Atom, Prop, &Atom1);
        else if (Atom == Atom2)
            ok(Prop == &Atom2, "Window %p Prop 0x%04x = %p, expected %p\n", hWnd, Atom, Prop, &Atom2);
        else
            ok(Prop == NULL, "Window %p Prop 0x%04x = %p\n", hWnd, Atom, Prop);
    }

    /* In particular we shouldn't see these from WM_SETICON */
    SysICAtom = RegisterWindowMessageW(L"SysIC");
    Prop = GetPropW(hWnd, (PCWSTR)MAKEINTATOM(SysICAtom));
    ok(Prop == NULL, "SysIC prop (0x%04x) is %p\n", SysICAtom, Prop);

    SysICSAtom = RegisterWindowMessageW(L"SysICS");
    Prop = GetPropW(hWnd, (PCWSTR)MAKEINTATOM(SysICSAtom));
    ok(Prop == NULL, "SysICS prop (0x%04x) is %p\n", SysICSAtom, Prop);

    GlobalDeleteAtom(Atom1);
    GlobalDeleteAtom(Atom2);

    DestroyWindow(hWnd);

    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        DispatchMessageA(&msg);
 }
