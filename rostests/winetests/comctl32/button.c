/* Unit test suite for Button control.
 *
 * Copyright 1999 Ove Kaaven
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2004, 2005 Dmitry Timoshkov
 * Copyright 2014 Nikolay Sivov for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

//#include <windows.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <commctrl.h>

#include "wine/test.h"
#include "v6util.h"
#include "msg.h"

#define IS_WNDPROC_HANDLE(x) (((ULONG_PTR)(x) >> 16) == (~0u >> 16))

static BOOL (WINAPI *pSetWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);
static BOOL (WINAPI *pRemoveWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR);
static LRESULT (WINAPI *pDefSubclassProc)(HWND, UINT, WPARAM, LPARAM);

/****************** button message test *************************/
#define ID_BUTTON 0x000e

#define COMBINED_SEQ_INDEX 0
#define NUM_MSG_SEQUENCES  1

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

struct wndclass_redirect_data
{
    ULONG size;
    DWORD res;
    ULONG name_len;
    ULONG name_offset;
    ULONG module_len;
    ULONG module_offset;
};

/* returned pointer is valid as long as activation context is alive */
static WCHAR* get_versioned_classname(const WCHAR *name)
{
    BOOL (WINAPI *pFindActCtxSectionStringW)(DWORD,const GUID *,ULONG,LPCWSTR,PACTCTX_SECTION_KEYED_DATA);
    struct wndclass_redirect_data *wnddata;
    ACTCTX_SECTION_KEYED_DATA data;
    BOOL ret;

    pFindActCtxSectionStringW = (void*)GetProcAddress(GetModuleHandleA("kernel32"), "FindActCtxSectionStringW");

    memset(&data, 0, sizeof(data));
    data.cbSize = sizeof(data);

    ret = pFindActCtxSectionStringW(0, NULL,
                                    ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION,
                                    name, &data);
    ok(ret, "got %d, error %u\n", ret, GetLastError());
    wnddata = (struct wndclass_redirect_data*)data.lpData;
    return (WCHAR*)((BYTE*)wnddata + wnddata->name_offset);
}

static void init_functions(void)
{
    HMODULE hmod = GetModuleHandleA("comctl32.dll");
    ok(hmod != NULL, "got %p\n", hmod);

#define MAKEFUNC_ORD(f, ord) (p##f = (void*)GetProcAddress(hmod, (LPSTR)(ord)))
    MAKEFUNC_ORD(SetWindowSubclass, 410);
    MAKEFUNC_ORD(RemoveWindowSubclass, 412);
    MAKEFUNC_ORD(DefSubclassProc, 413);
#undef MAKEFUNC_ORD
}

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
        diff = time - GetTickCount();
    }
}

static BOOL ignore_message( UINT message )
{
    /* these are always ignored */
    return (message >= 0xc000 ||
            message == WM_GETICON ||
            message == WM_GETOBJECT ||
            message == WM_TIMECHANGE ||
            message == WM_DISPLAYCHANGE ||
            message == WM_DEVICECHANGE ||
            message == WM_DWMNCRENDERINGCHANGED ||
            message == WM_GETTEXTLENGTH ||
            message == WM_GETTEXT);
}

static LRESULT CALLBACK button_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR ref_data)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    if (ignore_message( message )) return pDefSubclassProc(hwnd, message, wParam, lParam);

    switch (message)
    {
    case WM_SYNCPAINT:
        break;
    case BM_SETSTATE:
        if (GetCapture())
            ok(GetCapture() == hwnd, "GetCapture() = %p\n", GetCapture());
        /* fall through */
    default:
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        add_message(sequences, COMBINED_SEQ_INDEX, &msg);
    }

    if (message == WM_NCDESTROY)
        pRemoveWindowSubclass(hwnd, button_subclass_proc, 0);

    defwndproc_counter++;
    ret = pDefSubclassProc(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static LRESULT WINAPI test_parent_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    static LONG beginpaint_counter = 0;
    LRESULT ret;
    struct message msg;

    if (ignore_message( message )) return 0;

    if (message == WM_PARENTNOTIFY || message == WM_CANCELMODE ||
        message == WM_SETFOCUS || message == WM_KILLFOCUS ||
        message == WM_ENABLE || message == WM_ENTERIDLE ||
        message == WM_DRAWITEM || message == WM_COMMAND ||
        message == WM_IME_SETCONTEXT)
    {
        switch (message)
        {
            /* ignore */
            case WM_NCHITTEST:
                return HTCLIENT;
            case WM_SETCURSOR:
            case WM_MOUSEMOVE:
            case WM_NCMOUSEMOVE:
                return 0;
        }

        msg.message = message;
        msg.flags = sent|parent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        if (beginpaint_counter) msg.flags |= beginpaint;
        msg.wParam = wParam;
        msg.lParam = lParam;
        add_message(sequences, COMBINED_SEQ_INDEX, &msg);
    }

    if (message == WM_PAINT)
    {
        PAINTSTRUCT ps;
        beginpaint_counter++;
        BeginPaint( hwnd, &ps );
        beginpaint_counter--;
        EndPaint( hwnd, &ps );
        return 0;
    }

    defwndproc_counter++;
    ret = DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static const struct message setfocus_seq[] =
{
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { WM_SETFOCUS, sent|wparam },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_SETFOCUS) },
    { WM_APP, sent|wparam|lparam },
    { WM_PAINT, sent },
    { 0 }
};

static const struct message killfocus_seq[] =
{
    { WM_KILLFOCUS, sent|wparam, 0 },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_KILLFOCUS) },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 1 },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { 0 }
};

static const struct message setfocus_static_seq[] =
{
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { WM_SETFOCUS, sent|wparam, 0 },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_SETFOCUS) },
    { WM_COMMAND, sent|wparam|parent|optional, MAKEWPARAM(ID_BUTTON, BN_CLICKED) }, /* radio button */
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { 0 }
};

static const struct message setfocus_groupbox_seq[] =
{
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { WM_SETFOCUS, sent|wparam, 0 },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_SETFOCUS) },
    { WM_COMMAND, sent|wparam|parent|optional, MAKEWPARAM(ID_BUTTON, BN_CLICKED) }, /* radio button */
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { 0 }
};

static const struct message killfocus_static_seq[] =
{
    { WM_KILLFOCUS, sent|wparam, 0 },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_KILLFOCUS) },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 1 },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { 0 }
};

static const struct message setfocus_ownerdraw_seq[] =
{
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { WM_SETFOCUS, sent|wparam, 0 },
    { WM_DRAWITEM, sent|wparam|parent, ID_BUTTON },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_SETFOCUS) },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { 0 }
};

static const struct message killfocus_ownerdraw_seq[] =
{
    { WM_KILLFOCUS, sent|wparam, 0 },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_KILLFOCUS) },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 1 },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_DRAWITEM, sent|wparam|parent, ID_BUTTON },
    { 0 }
};

static const struct message lbuttondown_seq[] =
{
    { WM_LBUTTONDOWN, sent|wparam|lparam, 0, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    { BM_SETSTATE, sent|wparam|defwinproc, TRUE },
    { 0 }
};

static const struct message lbuttonup_seq[] =
{
    { WM_LBUTTONUP, sent|wparam|lparam, 0, 0 },
    { BM_SETSTATE, sent|wparam|defwinproc, FALSE },
    { WM_CAPTURECHANGED, sent|wparam|defwinproc, 0 },
    { WM_COMMAND, sent|wparam|defwinproc, 0 },
    { 0 }
};

static const struct message setfont_seq[] =
{
    { WM_SETFONT, sent },
    { 0 }
};

static const struct message setstyle_seq[] =
{
    { BM_SETSTYLE, sent },
    { WM_STYLECHANGING, sent|wparam|defwinproc, GWL_STYLE },
    { WM_STYLECHANGED, sent|wparam|defwinproc, GWL_STYLE },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|defwinproc|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { 0 }
};

static const struct message setstyle_static_seq[] =
{
    { BM_SETSTYLE, sent },
    { WM_STYLECHANGING, sent|wparam|defwinproc, GWL_STYLE },
    { WM_STYLECHANGED, sent|wparam|defwinproc, GWL_STYLE },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|defwinproc|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { 0 }
};

static const struct message setstyle_user_seq[] =
{
    { BM_SETSTYLE, sent },
    { WM_STYLECHANGING, sent|wparam|defwinproc, GWL_STYLE },
    { WM_STYLECHANGED, sent|wparam|defwinproc, GWL_STYLE },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|defwinproc|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { 0 }
};

static const struct message setstyle_ownerdraw_seq[] =
{
    { BM_SETSTYLE, sent },
    { WM_STYLECHANGING, sent|wparam|defwinproc, GWL_STYLE },
    { WM_STYLECHANGED, sent|wparam|defwinproc, GWL_STYLE },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { WM_DRAWITEM, sent|wparam|parent, ID_BUTTON },
    { 0 }
};

static const struct message setstate_seq[] =
{
    { BM_SETSTATE, sent },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { 0 }
};

static const struct message setstate_static_seq[] =
{
    { BM_SETSTATE, sent },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { 0 }
};

static const struct message setstate_user_seq[] =
{
    { BM_SETSTATE, sent },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_HILITE) },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { 0 }
};

static const struct message setstate_ownerdraw_seq[] =
{
    { BM_SETSTATE, sent },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { WM_DRAWITEM, sent|wparam|parent, ID_BUTTON },
    { 0 }
};

static const struct message clearstate_seq[] =
{
    { BM_SETSTATE, sent },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_UNHILITE) },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { 0 }
};

static const struct message clearstate_ownerdraw_seq[] =
{
    { BM_SETSTATE, sent },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { WM_DRAWITEM, sent|wparam|parent, ID_BUTTON },
    { 0 }
};

static const struct message setcheck_ignored_seq[] =
{
    { BM_SETCHECK, sent },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message setcheck_static_seq[] =
{
    { BM_SETCHECK, sent },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { 0 }
};

static const struct message setcheck_radio_seq[] =
{
    { BM_SETCHECK, sent },
    { WM_STYLECHANGING, sent|wparam|defwinproc, GWL_STYLE },
    { WM_STYLECHANGED, sent|wparam|defwinproc, GWL_STYLE },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message setcheck_radio_redraw_seq[] =
{
    { BM_SETCHECK, sent },
    { WM_STYLECHANGING, sent|wparam|defwinproc, GWL_STYLE },
    { WM_STYLECHANGED, sent|wparam|defwinproc, GWL_STYLE },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { 0 }
};

static HWND create_button(DWORD style, HWND parent)
{
    HMENU menuid = 0;
    HWND hwnd;

    if (parent)
    {
        style |= WS_CHILD|BS_NOTIFY;
        menuid = (HMENU)ID_BUTTON;
    }
    hwnd = CreateWindowExA(0, "Button", "test", style, 0, 0, 50, 14, parent, menuid, 0, NULL);
    ok(hwnd != NULL, "failed to create a button, 0x%08x, %p\n", style, parent);
    pSetWindowSubclass(hwnd, button_subclass_proc, 0, 0);
    return hwnd;
}

static void test_button_messages(void)
{
    static const struct
    {
        DWORD style;
        DWORD dlg_code;
        const struct message *setfocus;
        const struct message *killfocus;
        const struct message *setstyle;
        const struct message *setstate;
        const struct message *clearstate;
        const struct message *setcheck;
    } button[] = {
        { BS_PUSHBUTTON, DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON,
          setfocus_seq, killfocus_seq, setstyle_seq,
          setstate_seq, setstate_seq, setcheck_ignored_seq },
        { BS_DEFPUSHBUTTON, DLGC_BUTTON | DLGC_DEFPUSHBUTTON,
          setfocus_seq, killfocus_seq, setstyle_seq,
          setstate_seq, setstate_seq, setcheck_ignored_seq },
        { BS_CHECKBOX, DLGC_BUTTON,
          setfocus_static_seq, killfocus_static_seq, setstyle_static_seq,
          setstate_static_seq, setstate_static_seq, setcheck_static_seq },
        { BS_AUTOCHECKBOX, DLGC_BUTTON,
          setfocus_static_seq, killfocus_static_seq, setstyle_static_seq,
          setstate_static_seq, setstate_static_seq, setcheck_static_seq },
        { BS_RADIOBUTTON, DLGC_BUTTON | DLGC_RADIOBUTTON,
          setfocus_static_seq, killfocus_static_seq, setstyle_static_seq,
          setstate_static_seq, setstate_static_seq, setcheck_radio_redraw_seq },
        { BS_3STATE, DLGC_BUTTON,
          setfocus_static_seq, killfocus_static_seq, setstyle_static_seq,
          setstate_static_seq, setstate_static_seq, setcheck_static_seq },
        { BS_AUTO3STATE, DLGC_BUTTON,
          setfocus_static_seq, killfocus_static_seq, setstyle_static_seq,
          setstate_static_seq, setstate_static_seq, setcheck_static_seq },
        { BS_GROUPBOX, DLGC_STATIC,
          setfocus_groupbox_seq, killfocus_static_seq, setstyle_static_seq,
          setstate_static_seq, setstate_static_seq, setcheck_ignored_seq },
        { BS_USERBUTTON, DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON,
          setfocus_seq, killfocus_seq, setstyle_user_seq,
          setstate_user_seq, clearstate_seq, setcheck_ignored_seq },
        { BS_AUTORADIOBUTTON, DLGC_BUTTON | DLGC_RADIOBUTTON,
          setfocus_static_seq, killfocus_static_seq, setstyle_static_seq,
          setstate_static_seq, setstate_static_seq, setcheck_radio_redraw_seq },
        { BS_OWNERDRAW, DLGC_BUTTON,
          setfocus_ownerdraw_seq, killfocus_ownerdraw_seq, setstyle_ownerdraw_seq,
          setstate_ownerdraw_seq, clearstate_ownerdraw_seq, setcheck_ignored_seq },
    };
    const struct message *seq;
    unsigned int i;
    HWND hwnd, parent;
    DWORD dlg_code;
    HFONT zfont;
    BOOL todo;

    /* selection with VK_SPACE should capture button window */
    hwnd = create_button(BS_CHECKBOX | WS_VISIBLE | WS_POPUP, NULL);
    ok(hwnd != 0, "Failed to create button window\n");
    ReleaseCapture();
    SetFocus(hwnd);
    SendMessageA(hwnd, WM_KEYDOWN, VK_SPACE, 0);
    ok(GetCapture() == hwnd, "Should be captured on VK_SPACE WM_KEYDOWN\n");
    SendMessageA(hwnd, WM_KEYUP, VK_SPACE, 0);
    DestroyWindow(hwnd);

    parent = CreateWindowExA(0, "TestParentClass", "Test parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             100, 100, 200, 200, 0, 0, 0, NULL);
    ok(parent != 0, "Failed to create parent window\n");

    for (i = 0; i < sizeof(button)/sizeof(button[0]); i++)
    {
        MSG msg;
        DWORD style, state;

        trace("%d: button test sequence\n", i);
        hwnd = create_button(button[i].style, parent);

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY);
        /* XP turns a BS_USERBUTTON into BS_PUSHBUTTON */
        if (button[i].style == BS_USERBUTTON)
            ok(style == BS_PUSHBUTTON, "expected style BS_PUSHBUTTON got %x\n", style);
        else
            ok(style == button[i].style, "expected style %x got %x\n", button[i].style, style);

        dlg_code = SendMessageA(hwnd, WM_GETDLGCODE, 0, 0);
        ok(dlg_code == button[i].dlg_code, "%u: wrong dlg_code %08x\n", i, dlg_code);

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        SetFocus(0);
        flush_events();
        SetFocus(0);
        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        todo = button[i].style != BS_OWNERDRAW;
        ok(GetFocus() == 0, "expected focus 0, got %p\n", GetFocus());
        SetFocus(hwnd);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(sequences, COMBINED_SEQ_INDEX, button[i].setfocus, "SetFocus(hwnd) on a button", todo);

        todo = button[i].style == BS_OWNERDRAW;
        SetFocus(0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(sequences, COMBINED_SEQ_INDEX, button[i].killfocus, "SetFocus(0) on a button", todo);
        ok(GetFocus() == 0, "expected focus 0, got %p\n", GetFocus());

        SendMessageA(hwnd, BM_SETSTYLE, button[i].style | BS_BOTTOM, TRUE);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(sequences, COMBINED_SEQ_INDEX, button[i].setstyle, "BM_SETSTYLE on a button", TRUE);

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_VISIBLE | WS_CHILD | BS_NOTIFY);
        /* XP doesn't turn a BS_USERBUTTON into BS_PUSHBUTTON here! */
        ok(style == button[i].style, "expected style %04x got %04x\n", button[i].style, style);

        state = SendMessageA(hwnd, BM_GETSTATE, 0, 0);
        ok(state == 0, "expected state 0, got %04x\n", state);

        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        SendMessageA(hwnd, BM_SETSTATE, TRUE, 0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(sequences, COMBINED_SEQ_INDEX, button[i].setstate, "BM_SETSTATE/TRUE on a button", TRUE);

        state = SendMessageA(hwnd, BM_GETSTATE, 0, 0);
        ok(state == BST_PUSHED, "expected state 0x0004, got %04x\n", state);

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY | WS_VISIBLE);
        ok(style == button[i].style, "expected style %04x got %04x\n", button[i].style, style);

        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        SendMessageA(hwnd, BM_SETSTATE, FALSE, 0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(sequences, COMBINED_SEQ_INDEX, button[i].clearstate, "BM_SETSTATE/FALSE on a button", TRUE);

        state = SendMessageA(hwnd, BM_GETSTATE, 0, 0);
        ok(state == 0, "expected state 0, got %04x\n", state);

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY | WS_VISIBLE);
        ok(style == button[i].style, "expected style %04x got %04x\n", button[i].style, style);

        state = SendMessageA(hwnd, BM_GETCHECK, 0, 0);
        ok(state == BST_UNCHECKED, "expected BST_UNCHECKED, got %04x\n", state);

        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        if (button[i].style == BS_RADIOBUTTON ||
            button[i].style == BS_AUTORADIOBUTTON)
        {
            seq = setcheck_radio_seq;
            todo = TRUE;
        }
        else
        {
            seq = setcheck_ignored_seq;
            todo = FALSE;
        }
        SendMessageA(hwnd, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(sequences, COMBINED_SEQ_INDEX, seq, "BM_SETCHECK on a button", todo);

        state = SendMessageA(hwnd, BM_GETCHECK, 0, 0);
        ok(state == BST_UNCHECKED, "expected BST_UNCHECKED, got %04x\n", state);

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY | WS_VISIBLE);
        ok(style == button[i].style, "expected style %04x got %04x\n", button[i].style, style);

        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        SendMessageA(hwnd, BM_SETCHECK, BST_CHECKED, 0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);

        if (button[i].style == BS_PUSHBUTTON ||
            button[i].style == BS_DEFPUSHBUTTON ||
            button[i].style == BS_GROUPBOX ||
            button[i].style == BS_USERBUTTON ||
            button[i].style == BS_OWNERDRAW)
        {
            ok_sequence(sequences, COMBINED_SEQ_INDEX, button[i].setcheck, "BM_SETCHECK on a button", FALSE);
            state = SendMessageA(hwnd, BM_GETCHECK, 0, 0);
            ok(state == BST_UNCHECKED, "expected check BST_UNCHECKED, got %04x\n", state);
        }
        else
        {
            ok_sequence(sequences, COMBINED_SEQ_INDEX, button[i].setcheck, "BM_SETCHECK on a button", TRUE);
            state = SendMessageA(hwnd, BM_GETCHECK, 0, 0);
            ok(state == BST_CHECKED, "expected check BST_CHECKED, got %04x\n", state);
        }

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY | WS_VISIBLE);
        if (button[i].style == BS_RADIOBUTTON ||
            button[i].style == BS_AUTORADIOBUTTON)
            ok(style == (button[i].style | WS_TABSTOP), "expected style %04x | WS_TABSTOP got %04x\n", button[i].style, style);
        else
            ok(style == button[i].style, "expected style %04x got %04x\n", button[i].style, style);

        DestroyWindow(hwnd);
    }

    DestroyWindow(parent);

    hwnd = create_button(BS_PUSHBUTTON, NULL);

    SetForegroundWindow(hwnd);
    flush_events();

    SetActiveWindow(hwnd);
    SetFocus(0);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    SendMessageA(hwnd, WM_LBUTTONDOWN, 0, 0);
    ok_sequence(sequences, COMBINED_SEQ_INDEX, lbuttondown_seq, "WM_LBUTTONDOWN on a button", FALSE);

    SendMessageA(hwnd, WM_LBUTTONUP, 0, 0);
    ok_sequence(sequences, COMBINED_SEQ_INDEX, lbuttonup_seq, "WM_LBUTTONUP on a button", TRUE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    zfont = GetStockObject(SYSTEM_FONT);
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)zfont, TRUE);
    UpdateWindow(hwnd);
    ok_sequence(sequences, COMBINED_SEQ_INDEX, setfont_seq, "WM_SETFONT on a button", FALSE);

    DestroyWindow(hwnd);
}

static void test_button_class(void)
{
    static const WCHAR testW[] = {'t','e','s','t',0};
    WNDCLASSEXW exW, ex2W;
    WNDCLASSEXA exA;
    char buffA[100];
    WCHAR *nameW;
    HWND hwnd;
    BOOL ret;
    int len;

    ret = GetClassInfoExA(NULL, WC_BUTTONA, &exA);
    ok(ret, "got %d\n", ret);
todo_wine
    ok(IS_WNDPROC_HANDLE(exA.lpfnWndProc), "got %p\n", exA.lpfnWndProc);

    ret = GetClassInfoExW(NULL, WC_BUTTONW, &exW);
    ok(ret, "got %d\n", ret);
    ok(!IS_WNDPROC_HANDLE(exW.lpfnWndProc), "got %p\n", exW.lpfnWndProc);

    /* check that versioned class is also accessible */
    nameW = get_versioned_classname(WC_BUTTONW);
    ok(lstrcmpW(nameW, WC_BUTTONW), "got %s\n", wine_dbgstr_w(nameW));

    ret = GetClassInfoExW(NULL, nameW, &ex2W);
todo_wine
    ok(ret, "got %d\n", ret);
if (ret) /* TODO: remove once Wine is fixed */
    ok(ex2W.lpfnWndProc == exW.lpfnWndProc, "got %p, %p\n", exW.lpfnWndProc, ex2W.lpfnWndProc);

    /* Check reported class name */
    hwnd = create_button(BS_CHECKBOX, NULL);
    len = GetClassNameA(hwnd, buffA, sizeof(buffA));
    ok(len == strlen(buffA), "got %d\n", len);
    ok(!strcmp(buffA, "Button"), "got %s\n", buffA);

    len = RealGetWindowClassA(hwnd, buffA, sizeof(buffA));
    ok(len == strlen(buffA), "got %d\n", len);
    ok(!strcmp(buffA, "Button"), "got %s\n", buffA);
    DestroyWindow(hwnd);

    /* explicitly create with versioned class name */
    hwnd = CreateWindowExW(0, nameW, testW, BS_CHECKBOX, 0, 0, 50, 14, NULL, 0, 0, NULL);
todo_wine
    ok(hwnd != NULL, "failed to create a window %s\n", wine_dbgstr_w(nameW));
if (hwnd)
{
    len = GetClassNameA(hwnd, buffA, sizeof(buffA));
    ok(len == strlen(buffA), "got %d\n", len);
    ok(!strcmp(buffA, "Button"), "got %s\n", buffA);

    len = RealGetWindowClassA(hwnd, buffA, sizeof(buffA));
    ok(len == strlen(buffA), "got %d\n", len);
    ok(!strcmp(buffA, "Button"), "got %s\n", buffA);

    DestroyWindow(hwnd);
}
}

static void register_parent_class(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = test_parent_wndproc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "TestParentClass";
    RegisterClassA(&cls);
}

START_TEST(button)
{
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;

    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;

    register_parent_class();

    init_functions();
    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    test_button_class();
    test_button_messages();

    unload_v6_module(ctx_cookie, hCtx);
}
