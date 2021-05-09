/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for SubclassWindow/UnsubclassWindow
 * PROGRAMMER:      Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#ifdef HAVE_APITEST
    #include <apitest.h>
    #define ATLASSUME(x) /*empty*/
    #define ATLASSERT(x) /*empty*/
#else
    #include "atltest.h"
    #define ATLASSUME(x) do { \
        trace("ATLASSUME(%s) %s.\n", #x, ((x) ? "success" : "failure")); \
    } while (0)
    #define ATLASSERT(x) do { \
        trace("ATLASSERT(%s) %s.\n", #x, ((x) ? "success" : "failure")); \
    } while (0)
#endif

#include <atlbase.h>
#include <atlwin.h>

#ifdef _WIN64
    #define INVALID_HWND ((HWND)(ULONG_PTR)0xDEADBEEFDEADBEEFULL)
#else
    #define INVALID_HWND ((HWND)(ULONG_PTR)0xDEADBEEF)
#endif

static BOOL s_flag = TRUE;

class CMyCtrl1 : public CWindowImpl<CMyCtrl1, CWindow>
{
public:
    static LPCWSTR GetWndClassName()
    {
        if (s_flag)
            return L"EDIT";
        else
            return L"STATIC";
    }

    CMyCtrl1()
    {
    }
    virtual ~CMyCtrl1()
    {
    }

    BEGIN_MSG_MAP(CMyCtrl1)
    END_MSG_MAP()
};

class CMyCtrl2
    : public CContainedWindowT<CWindowImpl<CMyCtrl2, CWindow> >
{
public:
    static LPCWSTR GetWndClassName()
    {
        if (s_flag)
            return L"EDIT";
        else
            return L"STATIC";
    }

    CMyCtrl2() : CContainedWindowT<CWindowImpl<CMyCtrl2, CWindow> >(this)
    {
    }
    virtual ~CMyCtrl2()
    {
    }

    BEGIN_MSG_MAP(CMyCtrl2)
    END_MSG_MAP()
};

static HWND MyCreateWindow(DWORD style)
{
    return CreateWindowW(L"EDIT", NULL, style,
                         CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
                         NULL, NULL, GetModuleHandleW(NULL), NULL);
}

static LRESULT CALLBACK
MyWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

START_TEST(SubclassWindow)
{
    const DWORD style = WS_POPUPWINDOW | ES_MULTILINE;
    HWND hwnd1, hwnd2;
    WNDPROC fn1, fn2;
    BOOL b;
    trace("DefWindowProcA == %p\n", DefWindowProcA);
    trace("DefWindowProcW == %p\n", DefWindowProcW);
    trace("MyWindowProc == %p\n", MyWindowProc);

    //
    // Ctrl1
    //
    {
        CMyCtrl1 Ctrl1;
        s_flag = TRUE; // "EDIT"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl1.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl1.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl1.m_hWnd == hwnd1, "Ctrl1.m_hWnd was %p\n", Ctrl1.m_hWnd);
        fn1 = Ctrl1.m_pfnSuperWindowProc;
        ok(fn1 != DefWindowProc, "fn1 was %p\n", fn1);
        hwnd2 = Ctrl1.UnsubclassWindow();
        ok(hwnd1 == hwnd2, "hwnd1 != hwnd2\n");
        fn2 = Ctrl1.m_pfnSuperWindowProc;
        ok(fn1 != fn2, "fn1 == fn2\n");
        ok(fn2 == DefWindowProc, "fn2 was %p\n", fn2);
        DestroyWindow(hwnd2);
        ok(Ctrl1.m_hWnd == NULL, "hwnd != NULL\n");
    }

    {
        CMyCtrl1 Ctrl1;
        s_flag = TRUE; // "EDIT"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl1.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl1.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl1.m_hWnd == hwnd1, "Ctrl1.m_hWnd was %p\n", Ctrl1.m_hWnd);
        fn1 = Ctrl1.m_pfnSuperWindowProc;
        ok(fn1 != DefWindowProc, "fn1 was %p\n", fn1);
        DestroyWindow(hwnd1); // destroy now
        hwnd2 = Ctrl1.UnsubclassWindow();
        ok(hwnd2 == NULL, "hwnd2 was %p\n", hwnd2);
        fn2 = Ctrl1.m_pfnSuperWindowProc;
        ok(fn2 == fn1, "fn2 was %p\n", fn2);
        DestroyWindow(hwnd2);
        ok(Ctrl1.m_hWnd == NULL, "hwnd != NULL\n");
    }

    {
        CMyCtrl1 Ctrl1;
        s_flag = FALSE; // "STATIC"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl1.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl1.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl1.m_hWnd == hwnd1, "Ctrl1.m_hWnd was %p\n", Ctrl1.m_hWnd);
        fn1 = Ctrl1.m_pfnSuperWindowProc;
        ok(fn1 != DefWindowProc, "fn1 was %p\n", fn1);
        hwnd2 = Ctrl1.UnsubclassWindow();
        ok(hwnd1 == hwnd2, "hwnd1 != hwnd2\n");
        fn2 = Ctrl1.m_pfnSuperWindowProc;
        ok(fn1 != fn2, "fn1 == fn2\n");
        ok(fn2 == DefWindowProc, "fn2 was %p\n", fn2);
        DestroyWindow(hwnd2);
        ok(Ctrl1.m_hWnd == NULL, "hwnd != NULL\n");
    }

    {
        CMyCtrl1 Ctrl1;
        s_flag = FALSE; // "STATIC"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl1.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl1.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl1.m_hWnd == hwnd1, "Ctrl1.m_hWnd was %p\n", Ctrl1.m_hWnd);
        fn1 = Ctrl1.m_pfnSuperWindowProc;
        ok(fn1 != DefWindowProc, "fn1 was %p\n", fn1);
        DestroyWindow(hwnd1); // destroy now
        hwnd2 = Ctrl1.UnsubclassWindow();
        ok(hwnd2 == NULL, "hwnd2 was %p\n", hwnd2);
        fn2 = Ctrl1.m_pfnSuperWindowProc;
        ok(fn1 == fn2, "fn1 != fn2\n");
        DestroyWindow(hwnd2);
        ok(Ctrl1.m_hWnd == NULL, "hwnd != NULL\n");
    }

    {
        CMyCtrl1 Ctrl1;
        s_flag = TRUE; // "EDIT"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl1.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl1.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl1.m_hWnd == hwnd1, "Ctrl1.m_hWnd was %p\n", Ctrl1.m_hWnd);
        Ctrl1.m_pfnSuperWindowProc = MyWindowProc;
        hwnd2 = Ctrl1.UnsubclassWindow();
        ok(hwnd1 == hwnd2, "hwnd1 != hwnd2\n");
        fn2 = Ctrl1.m_pfnSuperWindowProc;
        ok(fn1 == fn2, "fn2 was %p\n", fn2);
        ok(fn2 == DefWindowProc, "fn2 was %p\n", fn2);
        DestroyWindow(hwnd2);
        ok(Ctrl1.m_hWnd == NULL, "hwnd != NULL\n");
    }

    //
    // Ctrl2 (Not Forced)
    //
    {
        CMyCtrl2 Ctrl2;
        s_flag = TRUE; // "EDIT"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl2.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl2.m_hWnd == hwnd1, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 != DefWindowProc, "fn1 was %p\n", fn1);
        hwnd2 = Ctrl2.UnsubclassWindow(FALSE);
        ok(hwnd1 == hwnd2, "hwnd1 != hwnd2\n");
        fn2 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 != fn2, "fn1 == fn2\n");
        ok(fn2 == DefWindowProc, "fn2 was %p\n", fn2);
        DestroyWindow(hwnd2);
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
    }

    {
        CMyCtrl2 Ctrl2;
        s_flag = TRUE; // "EDIT"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl2.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl2.m_hWnd == hwnd1, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 != DefWindowProc, "fn1 was %p\n", fn1);
        DestroyWindow(hwnd1); // destroy now
        hwnd2 = Ctrl2.UnsubclassWindow(FALSE);
        ok(hwnd2 == NULL, "hwnd2 was %p\n", hwnd2);
        fn2 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 == fn2, "fn1 == fn2\n");
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        DestroyWindow(hwnd2);
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
    }

    {
        CMyCtrl2 Ctrl2;
        s_flag = FALSE; // "STATIC"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl2.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl2.m_hWnd == hwnd1, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 != DefWindowProc, "fn1 was %p\n", fn1);
        hwnd2 = Ctrl2.UnsubclassWindow(FALSE);
        ok(hwnd1 == hwnd2, "hwnd1 != hwnd2\n");
        fn2 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 != fn2, "fn1 == fn2\n");
        ok(fn2 == DefWindowProc, "fn2 was %p\n", fn2);
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        DestroyWindow(hwnd2);
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
    }

    {
        CMyCtrl2 Ctrl2;
        s_flag = FALSE; // "STATIC"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl2.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl2.m_hWnd == hwnd1, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 != DefWindowProc, "fn1 was %p\n", fn1);
        DestroyWindow(hwnd1); // destroy now
        hwnd2 = Ctrl2.UnsubclassWindow(FALSE);
        ok(hwnd2 == NULL, "hwnd2 was %p\n", hwnd2);
        fn2 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn2 != DefWindowProc, "fn2 was %p\n", fn2); // ntdll.dll!NtdllEditWndProc_W
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        DestroyWindow(hwnd2);
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
    }

    {
        CMyCtrl2 Ctrl2;
        s_flag = TRUE; // "EDIT"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl2.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl2.m_hWnd == hwnd1, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        Ctrl2.m_pfnSuperWindowProc = MyWindowProc;
        hwnd2 = Ctrl2.UnsubclassWindow(FALSE);
        ok(hwnd1 == hwnd2, "hwnd1 != hwnd2\n");
        fn2 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn2 == DefWindowProc, "fn2 was %p\n", fn2);
        ok(Ctrl2.m_hWnd == NULL, "hwnd != NULL\n");
        DestroyWindow(hwnd2);
        ok(Ctrl2.m_hWnd == NULL, "hwnd != NULL\n");
    }

    //
    // Ctrl2 (Forced)
    //
    {
        CMyCtrl2 Ctrl2;
        s_flag = TRUE; // "EDIT"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl2.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl2.m_hWnd == hwnd1, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 != DefWindowProc, "fn1 was %p\n", fn1);
        hwnd2 = Ctrl2.UnsubclassWindow(TRUE);
        ok(hwnd1 == hwnd2, "hwnd1 != hwnd2\n");
        fn2 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 != fn2, "fn1 == fn2\n");
        ok(fn2 == DefWindowProc, "fn2 was %p\n", fn2);
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        DestroyWindow(hwnd2);
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
    }

    {
        CMyCtrl2 Ctrl2;
        s_flag = TRUE; // "EDIT"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl2.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl2.m_hWnd == hwnd1, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 != DefWindowProc, "fn1 was %p\n", fn1);
        DestroyWindow(hwnd1); // destroy now
        hwnd2 = Ctrl2.UnsubclassWindow(TRUE);
        ok(hwnd2 == NULL, "hwnd2 was %p\n", hwnd2);
        fn2 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn2 == DefWindowProc, "fn2 was %p\n", fn2);
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        DestroyWindow(hwnd2);
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
    }

    {
        CMyCtrl2 Ctrl2;
        s_flag = FALSE; // "STATIC"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl2.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl2.m_hWnd == hwnd1, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 != DefWindowProc, "fn1 was %p\n", fn1);
        hwnd2 = Ctrl2.UnsubclassWindow(TRUE);
        ok(hwnd1 == hwnd2, "hwnd1 != hwnd2\n");
        fn2 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 != fn2, "fn1 == fn2\n");
        ok(fn2 == DefWindowProc, "fn2 was %p\n", fn2);
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        DestroyWindow(hwnd2);
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
    }

    {
        CMyCtrl2 Ctrl2;
        s_flag = FALSE; // "STATIC"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl2.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl2.m_hWnd == hwnd1, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 != DefWindowProc, "fn1 was %p\n", fn1);
        DestroyWindow(hwnd1); // destroy now
        hwnd2 = Ctrl2.UnsubclassWindow(TRUE);
        ok(hwnd2 == NULL, "hwnd2 was %p\n", hwnd2);
        fn2 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn2 == DefWindowProc, "fn2 was %p\n", fn2);
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        DestroyWindow(hwnd2);
        ok(Ctrl2.m_hWnd == NULL, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
    }

    {
        CMyCtrl2 Ctrl2;
        s_flag = TRUE; // "EDIT"
        hwnd1 = MyCreateWindow(style);
        ok(hwnd1 != NULL, "hwnd1 was NULL\n");
        fn1 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn1 == DefWindowProc, "fn1 was %p\n", fn1);
        b = Ctrl2.SubclassWindow(hwnd1);
        ok_int(b, TRUE);
        ok(Ctrl2.m_hWnd == hwnd1, "Ctrl2.m_hWnd was %p\n", Ctrl2.m_hWnd);
        Ctrl2.m_pfnSuperWindowProc = MyWindowProc;
        hwnd2 = Ctrl2.UnsubclassWindow(TRUE);
        ok(hwnd1 == hwnd2, "hwnd1 != hwnd2\n");
        fn2 = Ctrl2.m_pfnSuperWindowProc;
        ok(fn2 == DefWindowProc, "fn2 was %p\n", fn2);
        ok(Ctrl2.m_hWnd == NULL, "hwnd != NULL\n");
        DestroyWindow(hwnd2);
        ok(Ctrl2.m_hWnd == NULL, "hwnd != NULL\n");
    }
}
