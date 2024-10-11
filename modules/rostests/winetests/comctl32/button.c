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

#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"
#include "v6util.h"
#include "msg.h"

#define IS_WNDPROC_HANDLE(x) (((ULONG_PTR)(x) >> 16) == (~0u >> 16))

static BOOL is_theme_active;

static BOOL (WINAPI *pSetWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);
static BOOL (WINAPI *pRemoveWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR);
static LRESULT (WINAPI *pDefSubclassProc)(HWND, UINT, WPARAM, LPARAM);
static HIMAGELIST (WINAPI *pImageList_Create)(int, int, UINT, int, int);
static int (WINAPI *pImageList_Add)(HIMAGELIST, HBITMAP, HBITMAP);
static BOOL (WINAPI *pImageList_Destroy)(HIMAGELIST);

/****************** button message test *************************/
#define ID_BUTTON 0x000e

#define COMBINED_SEQ_INDEX  0
#define PARENT_CD_SEQ_INDEX 1
#define NUM_MSG_SEQUENCES   2

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
    struct wndclass_redirect_data *wnddata;
    ACTCTX_SECTION_KEYED_DATA data;
    BOOL ret;

    memset(&data, 0, sizeof(data));
    data.cbSize = sizeof(data);
    ret = FindActCtxSectionStringW(0, NULL, ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION, name, &data);
    ok(ret, "Failed to find class redirection section, error %lu\n", GetLastError());
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

#define X(f) p##f = (void *)GetProcAddress(hmod, #f);
    X(ImageList_Create);
    X(ImageList_Add);
    X(ImageList_Destroy);
#undef X
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

static BOOL equal_dc(HDC hdc1, HDC hdc2, int width, int height)
{
    int x, y;

    for (x = 0; x < width; ++x)
    {
        for (y = 0; y < height; ++y)
        {
            if (GetPixel(hdc1, x, y) != GetPixel(hdc2, x, y))
                return FALSE;
        }
    }

    return TRUE;
}

static LRESULT CALLBACK button_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR ref_data)
{
    static LONG defwndproc_counter = 0;
    struct message msg = { 0 };
    LRESULT ret;

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

static struct
{
    DWORD button;
    UINT line;
    UINT state;
    DWORD ret;
    BOOL empty;
} test_cd;

#define set_test_cd_state(s) do { \
    test_cd.state = (s); \
    test_cd.empty = TRUE; \
    test_cd.line = __LINE__; \
} while (0)

#define set_test_cd_ret(r) do { \
    test_cd.ret = (r); \
    test_cd.empty = TRUE; \
    test_cd.line = __LINE__; \
} while (0)

static void disable_test_cd(void)
{
    test_cd.line = 0;
}

static LRESULT WINAPI test_parent_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    static LONG beginpaint_counter = 0;
    static HDC cd_first_hdc;
    struct message msg = { 0 };
    NMCUSTOMDRAW *cd = (NMCUSTOMDRAW*)lParam;
    NMBCDROPDOWN *bcd = (NMBCDROPDOWN*)lParam;
    LRESULT ret;

    if (ignore_message( message )) return 0;

    if (message == WM_PARENTNOTIFY || message == WM_CANCELMODE ||
        message == WM_SETFOCUS || message == WM_KILLFOCUS ||
        message == WM_ENABLE || message == WM_ENTERIDLE ||
        message == WM_DRAWITEM || message == WM_COMMAND ||
        message == WM_IME_SETCONTEXT)
    {
        msg.message = message;
        msg.flags = sent|parent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        if (beginpaint_counter) msg.flags |= beginpaint;
        msg.wParam = wParam;
        msg.lParam = lParam;
        add_message(sequences, COMBINED_SEQ_INDEX, &msg);
    }

    if (message == WM_NOTIFY && cd->hdr.code == NM_CUSTOMDRAW && test_cd.line)
    {
        /* Ignore an inconsistency across Windows versions */
        UINT state = cd->uItemState & ~CDIS_SHOWKEYBOARDCUES;

        /* Some Windows configurations paint twice with different DC */
        if (test_cd.empty)
        {
            cd_first_hdc = cd->hdc;
            test_cd.empty = FALSE;
        }

        ok_(__FILE__,test_cd.line)(!(cd->dwDrawStage & CDDS_ITEM),
            "[%lu] CDDS_ITEM is set\n", test_cd.button);

        ok_(__FILE__,test_cd.line)(state == test_cd.state,
            "[%lu] expected uItemState %u, got %u\n", test_cd.button,
            test_cd.state, state);

        msg.message = message;
        msg.flags = sent|parent|wparam|lparam|id|custdraw;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.id = NM_CUSTOMDRAW;
        msg.stage = cd->dwDrawStage;
        if (cd->hdc == cd_first_hdc)
            add_message(sequences, PARENT_CD_SEQ_INDEX, &msg);

        ret = test_cd.ret;
        switch (msg.stage)
        {
            case CDDS_PREERASE:
                ret &= ~CDRF_NOTIFYPOSTPAINT;
                cd->dwItemSpec = 0xdeadbeef;
                break;
            case CDDS_PREPAINT:
                ret &= ~CDRF_NOTIFYPOSTERASE;
                break;
            case CDDS_POSTERASE:
            case CDDS_POSTPAINT:
                ok_(__FILE__,test_cd.line)(cd->dwItemSpec == 0xdeadbeef,
                    "[%lu] NMCUSTOMDRAW was not shared, stage %lu\n", test_cd.button, msg.stage);
                break;
        }
        return ret;
    }

    if (message == WM_NOTIFY && bcd->hdr.code == BCN_DROPDOWN)
    {
        UINT button = GetWindowLongW(bcd->hdr.hwndFrom, GWL_STYLE) & BS_TYPEMASK;
        RECT rc;

        GetClientRect(bcd->hdr.hwndFrom, &rc);

        ok(bcd->hdr.hwndFrom != NULL, "Received BCN_DROPDOWN with no hwnd attached, wParam %Iu id %Iu\n",
           wParam, bcd->hdr.idFrom);
        ok(bcd->hdr.idFrom == wParam, "[%u] Mismatch between wParam (%Iu) and idFrom (%Iu)\n",
           button, wParam, bcd->hdr.idFrom);
        ok(EqualRect(&rc, &bcd->rcButton), "[%u] Wrong rcButton, expected %s got %s\n",
           button, wine_dbgstr_rect(&rc), wine_dbgstr_rect(&bcd->rcButton));

        msg.message = message;
        msg.flags = sent|parent|wparam|lparam|id;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.id = BCN_DROPDOWN;
        add_message(sequences, COMBINED_SEQ_INDEX, &msg);
        return 0;
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
    { BM_GETSTATE, sent|optional }, /* when touchscreen is present */
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
    { BM_GETSTATE, sent|optional }, /* when touchscreen is present */
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
    { BM_GETSTATE, sent|optional }, /* when touchscreen is present */
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
    { BM_GETSTATE, sent|optional }, /* when touchscreen is present */
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
    { BM_GETSTATE, sent|defwinproc|optional }, /* when touchscreen is present */
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
    { WM_PAINT, sent|optional },
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
    { WM_PAINT, sent|optional },
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
    { WM_PAINT, sent|optional },
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
    { WM_NCPAINT, sent|defwinproc|optional }, /* FIXME: Wine sends it */
    { 0 }
};

static const struct message empty_cd_seq[] = { { 0 } };

static const struct message pre_cd_seq[] =
{
    { WM_NOTIFY, sent|parent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_PREERASE },
    { 0 }
};

static const struct message pre_pre_cd_seq[] =
{
    { WM_NOTIFY, sent|parent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_PREERASE },
    { WM_NOTIFY, sent|parent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_PREPAINT },
    { 0 }
};

static const struct message pre_post_pre_cd_seq[] =
{
    { WM_NOTIFY, sent|parent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_PREERASE },
    { WM_NOTIFY, sent|parent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_POSTERASE },
    { WM_NOTIFY, sent|parent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_PREPAINT },
    { 0 }
};

static const struct message pre_pre_post_cd_seq[] =
{
    { WM_NOTIFY, sent|parent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_PREERASE },
    { WM_NOTIFY, sent|parent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_PREPAINT },
    { WM_NOTIFY, sent|parent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_POSTPAINT },
    { 0 }
};

static const struct message pre_post_pre_post_cd_seq[] =
{
    { WM_NOTIFY, sent|parent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_PREERASE },
    { WM_NOTIFY, sent|parent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_POSTERASE },
    { WM_NOTIFY, sent|parent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_PREPAINT },
    { WM_NOTIFY, sent|parent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_POSTPAINT },
    { 0 }
};

static const struct message bcn_dropdown_seq[] =
{
    { WM_KEYDOWN, sent|wparam|lparam, VK_DOWN, 0 },
    { BCM_SETDROPDOWNSTATE, sent|wparam|lparam|defwinproc, 1, 0 },
    { WM_NOTIFY, sent|parent|id, 0, 0, BCN_DROPDOWN },
    { BCM_SETDROPDOWNSTATE, sent|wparam|lparam|defwinproc, 0, 0 },
    { WM_KEYUP, sent|wparam|lparam, VK_DOWN, 0xc0000000 },
    { WM_PAINT, sent },
    { WM_DRAWITEM, sent|parent|optional },  /* for owner draw button */
    { WM_PAINT, sent|optional },            /* sometimes sent rarely */
    { WM_DRAWITEM, sent|parent|optional },
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
    hwnd = CreateWindowExA(0, WC_BUTTONA, "test", style, 0, 0, 50, 14, parent, menuid, 0, NULL);
    ok(hwnd != NULL, "failed to create a button, 0x%08lx, %p\n", style, parent);
    pSetWindowSubclass(hwnd, button_subclass_proc, 0, 0);
    return hwnd;
}

static void test_button_messages(void)
{
    enum cd_seq_type
    {
        cd_seq_empty,
        cd_seq_normal,
        cd_seq_optional
    };

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
        enum cd_seq_type cd_setfocus_type;
        enum cd_seq_type cd_setstyle_type;
        enum cd_seq_type cd_setstate_type;
        enum cd_seq_type cd_setcheck_type;
    } button[] = {
        { BS_PUSHBUTTON, DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON,
          setfocus_seq, killfocus_seq, setstyle_seq,
          setstate_seq, setstate_seq, setcheck_ignored_seq,
          cd_seq_normal, cd_seq_normal, cd_seq_normal, cd_seq_optional },
        { BS_DEFPUSHBUTTON, DLGC_BUTTON | DLGC_DEFPUSHBUTTON,
          setfocus_seq, killfocus_seq, setstyle_seq,
          setstate_seq, setstate_seq, setcheck_ignored_seq,
          cd_seq_normal, cd_seq_normal, cd_seq_normal, cd_seq_optional },
        { BS_CHECKBOX, DLGC_BUTTON,
          setfocus_static_seq, killfocus_static_seq, setstyle_static_seq,
          setstate_static_seq, setstate_static_seq, setcheck_static_seq,
          cd_seq_optional, cd_seq_optional, cd_seq_optional, cd_seq_optional },
        { BS_AUTOCHECKBOX, DLGC_BUTTON,
          setfocus_static_seq, killfocus_static_seq, setstyle_static_seq,
          setstate_static_seq, setstate_static_seq, setcheck_static_seq,
          cd_seq_optional, cd_seq_optional, cd_seq_optional, cd_seq_optional },
        { BS_RADIOBUTTON, DLGC_BUTTON | DLGC_RADIOBUTTON,
          setfocus_static_seq, killfocus_static_seq, setstyle_static_seq,
          setstate_static_seq, setstate_static_seq, setcheck_radio_redraw_seq,
          cd_seq_optional, cd_seq_optional, cd_seq_optional, cd_seq_optional },
        { BS_3STATE, DLGC_BUTTON,
          setfocus_static_seq, killfocus_static_seq, setstyle_static_seq,
          setstate_static_seq, setstate_static_seq, setcheck_static_seq,
          cd_seq_optional, cd_seq_optional, cd_seq_optional, cd_seq_optional },
        { BS_AUTO3STATE, DLGC_BUTTON,
          setfocus_static_seq, killfocus_static_seq, setstyle_static_seq,
          setstate_static_seq, setstate_static_seq, setcheck_static_seq,
          cd_seq_optional, cd_seq_optional, cd_seq_optional, cd_seq_optional },
        { BS_GROUPBOX, DLGC_STATIC,
          setfocus_groupbox_seq, killfocus_static_seq, setstyle_static_seq,
          setstate_static_seq, setstate_static_seq, setcheck_ignored_seq,
          cd_seq_empty, cd_seq_empty, cd_seq_empty, cd_seq_empty },
        { BS_USERBUTTON, DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON,
          setfocus_seq, killfocus_seq, setstyle_user_seq,
          setstate_user_seq, clearstate_seq, setcheck_ignored_seq,
          cd_seq_normal, cd_seq_empty, cd_seq_empty, cd_seq_empty },
        { BS_AUTORADIOBUTTON, DLGC_BUTTON | DLGC_RADIOBUTTON,
          setfocus_static_seq, killfocus_static_seq, setstyle_static_seq,
          setstate_static_seq, setstate_static_seq, setcheck_radio_redraw_seq,
          cd_seq_optional, cd_seq_optional, cd_seq_optional, cd_seq_optional },
        { BS_OWNERDRAW, DLGC_BUTTON,
          setfocus_ownerdraw_seq, killfocus_ownerdraw_seq, setstyle_ownerdraw_seq,
          setstate_ownerdraw_seq, clearstate_ownerdraw_seq, setcheck_ignored_seq,
          cd_seq_empty, cd_seq_empty, cd_seq_empty, cd_seq_empty },
        { BS_SPLITBUTTON, DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON | DLGC_WANTARROWS,
          setfocus_seq, killfocus_seq, setstyle_seq,
          setstate_seq, setstate_seq, setcheck_ignored_seq,
          cd_seq_optional, cd_seq_optional, cd_seq_optional, cd_seq_empty },
        { BS_DEFSPLITBUTTON, DLGC_BUTTON | DLGC_DEFPUSHBUTTON | DLGC_WANTARROWS,
          setfocus_seq, killfocus_seq, setstyle_seq,
          setstate_seq, setstate_seq, setcheck_ignored_seq,
          cd_seq_optional, cd_seq_optional, cd_seq_optional, cd_seq_empty },
        { BS_COMMANDLINK, DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON,
          setfocus_seq, killfocus_seq, setstyle_seq,
          setstate_seq, setstate_seq, setcheck_ignored_seq,
          cd_seq_optional, cd_seq_optional, cd_seq_optional, cd_seq_empty },
        { BS_DEFCOMMANDLINK, DLGC_BUTTON | DLGC_DEFPUSHBUTTON,
          setfocus_seq, killfocus_seq, setstyle_seq,
          setstate_seq, setstate_seq, setcheck_ignored_seq,
          cd_seq_optional, cd_seq_optional, cd_seq_optional, cd_seq_empty }
    };
    LOGFONTA logfont = { 0 };
    const struct message *seq, *cd_seq;
    HFONT zfont, hfont2;
    unsigned int i;
    HWND hwnd, parent;
    DWORD dlg_code;
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

    logfont.lfHeight = -12;
    logfont.lfWeight = FW_NORMAL;
    strcpy(logfont.lfFaceName, "Tahoma");

    hfont2 = CreateFontIndirectA(&logfont);
    ok(hfont2 != NULL, "Failed to create Tahoma font\n");

#define check_cd_seq(type, context) do { \
        if (button[i].type != cd_seq_optional || !test_cd.empty) \
            ok_sequence(sequences, PARENT_CD_SEQ_INDEX, cd_seq, "[CustomDraw] " context, FALSE); \
    } while(0)

    for (i = 0; i < ARRAY_SIZE(button); i++)
    {
        HFONT prevfont, hfont;
        MSG msg;
        DWORD style, state;
        HDC hdc;

        test_cd.button = button[i].style;
        hwnd = create_button(button[i].style, parent);
        ok(hwnd != NULL, "Failed to create a button.\n");

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY);
        /* XP turns a BS_USERBUTTON into BS_PUSHBUTTON */
        if (button[i].style == BS_USERBUTTON)
            ok(style == BS_PUSHBUTTON, "expected style BS_PUSHBUTTON got %lx\n", style);
        else
            ok(style == button[i].style, "expected style %lx got %lx\n", button[i].style, style);

        dlg_code = SendMessageA(hwnd, WM_GETDLGCODE, 0, 0);
        if (button[i].style == BS_SPLITBUTTON ||
                button[i].style == BS_DEFSPLITBUTTON ||
                button[i].style == BS_COMMANDLINK ||
                button[i].style == BS_DEFCOMMANDLINK)
        {
            ok(dlg_code == button[i].dlg_code || broken(dlg_code == DLGC_BUTTON) /* WinXP */, "%u: wrong dlg_code %08lx\n", i, dlg_code);
        }
        else
            ok(dlg_code == button[i].dlg_code, "%u: wrong dlg_code %08lx\n", i, dlg_code);

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        SetFocus(0);
        flush_events();
        SetFocus(0);
        cd_seq = (button[i].cd_setfocus_type == cd_seq_empty) ? empty_cd_seq : pre_pre_cd_seq;
        flush_sequences(sequences, NUM_MSG_SEQUENCES);
        set_test_cd_ret(CDRF_DODEFAULT);
        set_test_cd_state(CDIS_FOCUS);

        ok(GetFocus() == 0, "expected focus 0, got %p\n", GetFocus());
        SetFocus(hwnd);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(sequences, COMBINED_SEQ_INDEX, button[i].setfocus, "SetFocus(hwnd) on a button", FALSE);
        check_cd_seq(cd_setfocus_type, "SetFocus(hwnd)");

        set_test_cd_state(0);
        SetFocus(0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(sequences, COMBINED_SEQ_INDEX, button[i].killfocus, "SetFocus(0) on a button", FALSE);
        check_cd_seq(cd_setfocus_type, "SetFocus(0)");
        ok(GetFocus() == 0, "expected focus 0, got %p\n", GetFocus());

        cd_seq = (button[i].cd_setstyle_type == cd_seq_empty) ? empty_cd_seq : pre_pre_cd_seq;
        set_test_cd_state(0);

        SendMessageA(hwnd, BM_SETSTYLE, button[i].style | BS_BOTTOM, TRUE);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        todo = button[i].style == BS_OWNERDRAW;
        ok_sequence(sequences, COMBINED_SEQ_INDEX, button[i].setstyle, "BM_SETSTYLE on a button", todo);
        check_cd_seq(cd_setstyle_type, "BM_SETSTYLE");

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_VISIBLE | WS_CHILD | BS_NOTIFY);
        /* XP doesn't turn a BS_USERBUTTON into BS_PUSHBUTTON here! */
        ok(style == button[i].style, "expected style %04lx got %04lx\n", button[i].style, style);

        state = SendMessageA(hwnd, BM_GETSTATE, 0, 0);
        ok(state == 0, "expected state 0, got %04lx\n", state);

        cd_seq = (button[i].cd_setstate_type == cd_seq_empty) ? empty_cd_seq : pre_pre_cd_seq;
        flush_sequences(sequences, NUM_MSG_SEQUENCES);
        set_test_cd_state(CDIS_SELECTED);

        SendMessageA(hwnd, BM_SETSTATE, TRUE, 0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(sequences, COMBINED_SEQ_INDEX, button[i].setstate, "BM_SETSTATE/TRUE on a button", FALSE);
        check_cd_seq(cd_setstate_type, "BM_SETSTATE/TRUE");

        state = SendMessageA(hwnd, BM_GETSTATE, 0, 0);
        ok(state == BST_PUSHED, "expected state 0x0004, got %04lx\n", state);

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY | WS_VISIBLE);
        ok(style == button[i].style, "expected style %04lx got %04lx\n", button[i].style, style);

        flush_sequences(sequences, NUM_MSG_SEQUENCES);
        set_test_cd_state(0);

        SendMessageA(hwnd, BM_SETSTATE, FALSE, 0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(sequences, COMBINED_SEQ_INDEX, button[i].clearstate, "BM_SETSTATE/FALSE on a button", FALSE);
        check_cd_seq(cd_setstate_type, "BM_SETSTATE/FALSE");

        state = SendMessageA(hwnd, BM_GETSTATE, 0, 0);
        ok(state == 0, "expected state 0, got %04lx\n", state);

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY | WS_VISIBLE);
        ok(style == button[i].style, "expected style %04lx got %04lx\n", button[i].style, style);

        state = SendMessageA(hwnd, BM_GETCHECK, 0, 0);
        ok(state == BST_UNCHECKED, "expected BST_UNCHECKED, got %04lx\n", state);

        cd_seq = (button[i].cd_setcheck_type == cd_seq_empty) ? empty_cd_seq : pre_pre_cd_seq;
        flush_sequences(sequences, NUM_MSG_SEQUENCES);
        set_test_cd_state(0);

        if (button[i].style == BS_RADIOBUTTON ||
            button[i].style == BS_AUTORADIOBUTTON)
        {
            seq = setcheck_radio_seq;
        }
        else
            seq = setcheck_ignored_seq;

        SendMessageA(hwnd, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(sequences, COMBINED_SEQ_INDEX, seq, "BM_SETCHECK on a button", FALSE);
        check_cd_seq(cd_setcheck_type, "BM_SETCHECK");

        state = SendMessageA(hwnd, BM_GETCHECK, 0, 0);
        ok(state == BST_UNCHECKED, "expected BST_UNCHECKED, got %04lx\n", state);

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY | WS_VISIBLE);
        ok(style == button[i].style, "expected style %04lx got %04lx\n", button[i].style, style);

        flush_sequences(sequences, NUM_MSG_SEQUENCES);
        set_test_cd_state(0);

        SendMessageA(hwnd, BM_SETCHECK, BST_CHECKED, 0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(sequences, COMBINED_SEQ_INDEX, button[i].setcheck, "BM_SETCHECK on a button", FALSE);
        check_cd_seq(cd_setcheck_type, "BM_SETCHECK");

        state = SendMessageA(hwnd, BM_GETCHECK, 0, 0);
        if (button[i].style == BS_PUSHBUTTON ||
            button[i].style == BS_DEFPUSHBUTTON ||
            button[i].style == BS_GROUPBOX ||
            button[i].style == BS_USERBUTTON ||
            button[i].style == BS_OWNERDRAW ||
            button[i].style == BS_SPLITBUTTON ||
            button[i].style == BS_DEFSPLITBUTTON ||
            button[i].style == BS_COMMANDLINK ||
            button[i].style == BS_DEFCOMMANDLINK)
        {
            ok(state == BST_UNCHECKED, "expected check BST_UNCHECKED, got %04lx\n", state);
        }
        else
            ok(state == BST_CHECKED, "expected check BST_CHECKED, got %04lx\n", state);

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY | WS_VISIBLE);
        if (button[i].style == BS_RADIOBUTTON ||
            button[i].style == BS_AUTORADIOBUTTON)
            ok(style == (button[i].style | WS_TABSTOP), "expected style %04lx | WS_TABSTOP got %04lx\n", button[i].style, style);
        else
            ok(style == button[i].style, "expected style %04lx got %04lx\n", button[i].style, style);

        /* Test that original font is not selected back after painting */
        hfont = (HFONT)SendMessageA(hwnd, WM_GETFONT, 0, 0);
        ok(hfont == NULL, "Unexpected control font.\n");

        SendMessageA(hwnd, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FONT), 0);

        hdc = CreateCompatibleDC(0);

        prevfont = SelectObject(hdc, hfont2);
        SendMessageA(hwnd, WM_PRINTCLIENT, (WPARAM)hdc, 0);
        ok(hfont2 != GetCurrentObject(hdc, OBJ_FONT) || broken(hfont2 == GetCurrentObject(hdc, OBJ_FONT)) /* WinXP */,
            "button[%u]: unexpected font selected after WM_PRINTCLIENT\n", i);
        SelectObject(hdc, prevfont);

        prevfont = SelectObject(hdc, hfont2);
        SendMessageA(hwnd, WM_PAINT, (WPARAM)hdc, 0);
        ok(hfont2 != GetCurrentObject(hdc, OBJ_FONT) || broken(hfont2 == GetCurrentObject(hdc, OBJ_FONT)) /* WinXP */,
            "button[%u]: unexpected font selected after WM_PAINT\n", i);
        SelectObject(hdc, prevfont);

        DeleteDC(hdc);

        /* Test Custom Draw return values */
        if (button[i].cd_setfocus_type != cd_seq_empty &&
            broken(button[i].style != BS_USERBUTTON) /* WinXP */)
        {
            static const struct
            {
                const char *context;
                LRESULT val;
                const struct message *seq;
            } ret[] = {
                { "CDRF_DODEFAULT", CDRF_DODEFAULT, pre_pre_cd_seq },
                { "CDRF_DOERASE", CDRF_DOERASE, pre_pre_cd_seq },
                { "CDRF_SKIPDEFAULT", CDRF_SKIPDEFAULT, pre_cd_seq },
                { "CDRF_SKIPDEFAULT | CDRF_NOTIFYPOSTERASE | CDRF_NOTIFYPOSTPAINT",
                   CDRF_SKIPDEFAULT | CDRF_NOTIFYPOSTERASE | CDRF_NOTIFYPOSTPAINT, pre_cd_seq },
                { "CDRF_NOTIFYPOSTERASE", CDRF_NOTIFYPOSTERASE, pre_post_pre_cd_seq },
                { "CDRF_NOTIFYPOSTPAINT", CDRF_NOTIFYPOSTPAINT, pre_pre_post_cd_seq },
                { "CDRF_NOTIFYPOSTERASE | CDRF_NOTIFYPOSTPAINT",
                   CDRF_NOTIFYPOSTERASE | CDRF_NOTIFYPOSTPAINT, pre_post_pre_post_cd_seq },
            };
            UINT k;

            for (k = 0; k < ARRAY_SIZE(ret); k++)
            {
                disable_test_cd();
                SetFocus(0);
                set_test_cd_ret(ret[k].val);
                set_test_cd_state(CDIS_FOCUS);
                SetFocus(hwnd);
                flush_sequences(sequences, NUM_MSG_SEQUENCES);

                while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
                if (button[i].cd_setfocus_type != cd_seq_optional || !test_cd.empty)
                    ok_sequence(sequences, PARENT_CD_SEQ_INDEX, ret[k].seq, ret[k].context, FALSE);
            }
        }

        disable_test_cd();

        if (!broken(LOBYTE(LOWORD(GetVersion())) < 6))  /* not available pre-Vista */
        {
            /* Send down arrow key to make the buttons send the drop down notification */
            flush_sequences(sequences, NUM_MSG_SEQUENCES);
            SendMessageW(hwnd, WM_KEYDOWN, VK_DOWN, 0);
            SendMessageW(hwnd, WM_KEYUP, VK_DOWN, 0xc0000000);
            while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
            ok_sequence(sequences, COMBINED_SEQ_INDEX, bcn_dropdown_seq, "BCN_DROPDOWN from the button", FALSE);
        }

        DestroyWindow(hwnd);
    }

#undef check_cd_seq

    DeleteObject(hfont2);
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
    WNDCLASSEXW exW, ex2W;
    WNDCLASSEXA exA;
    char buffA[100];
    WCHAR *nameW;
    HWND hwnd;
    BOOL ret;
    int len;

    ret = GetClassInfoExA(NULL, WC_BUTTONA, &exA);
    ok(ret, "got %d\n", ret);
    ok(IS_WNDPROC_HANDLE(exA.lpfnWndProc), "got %p\n", exA.lpfnWndProc);
    ok(exA.cbClsExtra == 0, "Unexpected class bytes %d.\n", exA.cbClsExtra);
    ok(exA.cbWndExtra == sizeof(void *), "Unexpected window bytes %d.\n", exA.cbWndExtra);

    ret = GetClassInfoExW(NULL, WC_BUTTONW, &exW);
    ok(ret, "got %d\n", ret);
    ok(!IS_WNDPROC_HANDLE(exW.lpfnWndProc), "got %p\n", exW.lpfnWndProc);
    ok(exW.cbClsExtra == 0, "Unexpected class bytes %d.\n", exW.cbClsExtra);
    ok(exW.cbWndExtra == sizeof(void *), "Unexpected window bytes %d.\n", exW.cbWndExtra);

    /* check that versioned class is also accessible */
    nameW = get_versioned_classname(WC_BUTTONW);
    ok(lstrcmpW(nameW, WC_BUTTONW), "got %s\n", wine_dbgstr_w(nameW));

    ret = GetClassInfoExW(NULL, nameW, &ex2W);
    ok(ret, "got %d\n", ret);
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
    hwnd = CreateWindowExW(0, nameW, L"test", BS_CHECKBOX, 0, 0, 50, 14, NULL, 0, 0, NULL);
    ok(hwnd != NULL, "failed to create a window %s\n", wine_dbgstr_w(nameW));

    len = GetClassNameA(hwnd, buffA, sizeof(buffA));
    ok(len == strlen(buffA), "got %d\n", len);
    ok(!strcmp(buffA, "Button"), "got %s\n", buffA);

    len = RealGetWindowClassA(hwnd, buffA, sizeof(buffA));
    ok(len == strlen(buffA), "got %d\n", len);
    ok(!strcmp(buffA, "Button"), "got %s\n", buffA);

    DestroyWindow(hwnd);
}

static void test_note(void)
{
    HWND hwnd;
    BOOL ret;
    WCHAR test_w[] = {'t', 'e', 's', 't', 0};
    WCHAR tes_w[] = {'t', 'e', 's', 0};
    WCHAR deadbeef_w[] = {'d', 'e', 'a', 'd', 'b', 'e', 'e', 'f', 0};
    WCHAR buffer_w[10];
    DWORD size;
    DWORD error;
    INT type;

    hwnd = create_button(BS_COMMANDLINK, NULL);
    ok(hwnd != NULL, "Expect hwnd not null\n");
    SetLastError(0xdeadbeef);
    size = ARRAY_SIZE(buffer_w);
    ret = SendMessageA(hwnd, BCM_GETNOTE, (WPARAM)&size, (LPARAM)buffer_w);
    error = GetLastError();
    if (!ret && error == 0xdeadbeef)
    {
        win_skip("BCM_GETNOTE message is unavailable. Skipping note tests\n"); /* xp or 2003 */
        DestroyWindow(hwnd);
        return;
    }
    DestroyWindow(hwnd);

    for (type = BS_PUSHBUTTON; type <= BS_DEFCOMMANDLINK; type++)
    {
        if (type == BS_DEFCOMMANDLINK || type == BS_COMMANDLINK)
        {
            hwnd = create_button(type, NULL);
            ok(hwnd != NULL, "Expect hwnd not null\n");

            /* Get note when note hasn't been not set yet */
            SetLastError(0xdeadbeef);
            lstrcpyW(buffer_w, deadbeef_w);
            size = ARRAY_SIZE(buffer_w);
            ret = SendMessageA(hwnd, BCM_GETNOTE, (WPARAM)&size, (LPARAM)buffer_w);
            error = GetLastError();
            ok(!ret, "Expect BCM_GETNOTE return false\n");
            ok(!lstrcmpW(buffer_w, deadbeef_w), "Expect note: %s, got: %s\n",
               wine_dbgstr_w(deadbeef_w), wine_dbgstr_w(buffer_w));
            ok(size == ARRAY_SIZE(buffer_w), "Got: %ld\n", size);
            ok(error == ERROR_INVALID_PARAMETER, "Expect last error: 0x%08x, got: 0x%08lx\n",
               ERROR_INVALID_PARAMETER, error);

            /* Get note length when note is not set */
            ret = SendMessageA(hwnd, BCM_GETNOTELENGTH, 0, 0);
            ok(ret == 0, "Expect note length: %d, got: %d\n", 0, ret);

            /* Successful set note, get note and get note length */
            SetLastError(0xdeadbeef);
            ret = SendMessageA(hwnd, BCM_SETNOTE, 0, (LPARAM)test_w);
            ok(ret, "Expect BCM_SETNOTE return true\n");
            error = GetLastError();
            ok(error == NO_ERROR, "Expect last error: 0x%08x, got: 0x%08lx\n", NO_ERROR, error);

            SetLastError(0xdeadbeef);
            lstrcpyW(buffer_w, deadbeef_w);
            size = ARRAY_SIZE(buffer_w);
            ret = SendMessageA(hwnd, BCM_GETNOTE, (WPARAM)&size, (LPARAM)buffer_w);
            ok(ret, "Expect BCM_GETNOTE return true\n");
            ok(!lstrcmpW(buffer_w, test_w), "Expect note: %s, got: %s\n", wine_dbgstr_w(test_w),
               wine_dbgstr_w(buffer_w));
            ok(size == ARRAY_SIZE(buffer_w), "Got: %ld\n", size);
            error = GetLastError();
            ok(error == NO_ERROR, "Expect last error: 0x%08x, got: 0x%08lx\n", NO_ERROR, error);

            ret = SendMessageA(hwnd, BCM_GETNOTELENGTH, 0, 0);
            ok(ret == ARRAY_SIZE(test_w) - 1, "Got: %d\n", ret);

            /* Insufficient buffer, return partial string */
            SetLastError(0xdeadbeef);
            lstrcpyW(buffer_w, deadbeef_w);
            size = ARRAY_SIZE(test_w) - 1;
            ret = SendMessageA(hwnd, BCM_GETNOTE, (WPARAM)&size, (LPARAM)buffer_w);
            ok(!ret, "Expect BCM_GETNOTE return false\n");
            ok(!lstrcmpW(buffer_w, tes_w), "Expect note: %s, got: %s\n", wine_dbgstr_w(tes_w),
               wine_dbgstr_w(buffer_w));
            ok(size == ARRAY_SIZE(test_w), "Got: %ld\n", size);
            error = GetLastError();
            ok(error == ERROR_INSUFFICIENT_BUFFER, "Expect last error: 0x%08x, got: 0x%08lx\n",
               ERROR_INSUFFICIENT_BUFFER, error);

            /* Set note with NULL buffer */
            SetLastError(0xdeadbeef);
            ret = SendMessageA(hwnd, BCM_SETNOTE, 0, 0);
            ok(ret, "Expect BCM_SETNOTE return false\n");
            error = GetLastError();
            ok(error == NO_ERROR, "Expect last error: 0x%08x, got: 0x%08lx\n", NO_ERROR, error);

            /* Check that set note with NULL buffer make note empty */
            SetLastError(0xdeadbeef);
            lstrcpyW(buffer_w, deadbeef_w);
            size = ARRAY_SIZE(buffer_w);
            ret = SendMessageA(hwnd, BCM_GETNOTE, (WPARAM)&size, (LPARAM)buffer_w);
            ok(ret, "Expect BCM_GETNOTE return true\n");
            ok(!*buffer_w, "Expect note length 0\n");
            ok(size == ARRAY_SIZE(buffer_w), "Got: %ld\n", size);
            error = GetLastError();
            ok(error == NO_ERROR, "Expect last error: 0x%08x, got: 0x%08lx\n", NO_ERROR, error);
            ret = SendMessageA(hwnd, BCM_GETNOTELENGTH, 0, 0);
            ok(ret == 0, "Expect note length: %d, got: %d\n", 0, ret);

            /* Get note with NULL buffer */
            SetLastError(0xdeadbeef);
            size = ARRAY_SIZE(buffer_w);
            ret = SendMessageA(hwnd, BCM_GETNOTE, (WPARAM)&size, 0);
            ok(!ret, "Expect BCM_SETNOTE return false\n");
            ok(size == ARRAY_SIZE(buffer_w), "Got: %ld\n", size);
            error = GetLastError();
            ok(error == ERROR_INVALID_PARAMETER, "Expect last error: 0x%08x, got: 0x%08lx\n",
               ERROR_INVALID_PARAMETER, error);

            /* Get note with NULL size */
            SetLastError(0xdeadbeef);
            lstrcpyW(buffer_w, deadbeef_w);
            ret = SendMessageA(hwnd, BCM_GETNOTE, 0, (LPARAM)buffer_w);
            ok(!ret, "Expect BCM_SETNOTE return false\n");
            ok(!lstrcmpW(buffer_w, deadbeef_w), "Expect note: %s, got: %s\n",
               wine_dbgstr_w(deadbeef_w), wine_dbgstr_w(buffer_w));
            error = GetLastError();
            ok(error == ERROR_INVALID_PARAMETER, "Expect last error: 0x%08x, got: 0x%08lx\n",
               ERROR_INVALID_PARAMETER, error);

            /* Get note with zero size */
            SetLastError(0xdeadbeef);
            size = 0;
            lstrcpyW(buffer_w, deadbeef_w);
            ret = SendMessageA(hwnd, BCM_GETNOTE, (WPARAM)&size, (LPARAM)buffer_w);
            ok(!ret, "Expect BCM_GETNOTE return false\n");
            ok(!lstrcmpW(buffer_w, deadbeef_w), "Expect note: %s, got: %s\n",
               wine_dbgstr_w(deadbeef_w), wine_dbgstr_w(buffer_w));
            ok(size == 1, "Got: %ld\n", size);
            error = GetLastError();
            ok(error == ERROR_INSUFFICIENT_BUFFER, "Expect last error: 0x%08x, got: 0x%08lx\n",
               ERROR_INSUFFICIENT_BUFFER, error);

            DestroyWindow(hwnd);
        }
        else
        {
            hwnd = create_button(type, NULL);
            ok(hwnd != NULL, "Expect hwnd not null\n");
            SetLastError(0xdeadbeef);
            size = ARRAY_SIZE(buffer_w);
            ret = SendMessageA(hwnd, BCM_GETNOTE, (WPARAM)&size, (LPARAM)buffer_w);
            ok(!ret, "Expect BCM_GETNOTE return false\n");
            error = GetLastError();
            ok(error == ERROR_NOT_SUPPORTED, "Expect last error: 0x%08x, got: 0x%08lx\n",
               ERROR_NOT_SUPPORTED, error);
            DestroyWindow(hwnd);
        }
    }
}

static void test_bm_get_set_image(void)
{
    HWND hwnd;
    HDC hdc;
    HBITMAP hbmp1x1;
    HBITMAP hbmp2x2;
    HBITMAP hmask2x2;
    ICONINFO icon_info2x2;
    HICON hicon2x2;
    HBITMAP hbmp;
    HICON hicon;
    ICONINFO icon_info;
    BITMAP bm;
    static const DWORD default_style = BS_PUSHBUTTON | WS_TABSTOP | WS_POPUP | WS_VISIBLE;

    hdc = GetDC(0);
    hbmp1x1 = CreateCompatibleBitmap(hdc, 1, 1);
    hbmp2x2 = CreateCompatibleBitmap(hdc, 2, 2);
    ZeroMemory(&bm, sizeof(bm));
    ok(GetObjectW(hbmp1x1, sizeof(bm), &bm), "Expect GetObjectW() success\n");
    ok(bm.bmWidth == 1 && bm.bmHeight == 1, "Expect bitmap size: %d,%d, got: %d,%d\n", 1, 1,
       bm.bmWidth, bm.bmHeight);
    ZeroMemory(&bm, sizeof(bm));
    ok(GetObjectW(hbmp2x2, sizeof(bm), &bm), "Expect GetObjectW() success\n");
    ok(bm.bmWidth == 2 && bm.bmHeight == 2, "Expect bitmap size: %d,%d, got: %d,%d\n", 2, 2,
       bm.bmWidth, bm.bmHeight);

    hmask2x2 = CreateCompatibleBitmap(hdc, 2, 2);
    ZeroMemory(&icon_info2x2, sizeof(icon_info2x2));
    icon_info2x2.fIcon = TRUE;
    icon_info2x2.hbmMask = hmask2x2;
    icon_info2x2.hbmColor = hbmp2x2;
    hicon2x2 = CreateIconIndirect(&icon_info2x2);
    ok(hicon2x2 !=NULL, "Expect CreateIconIndirect() success\n");

    ZeroMemory(&icon_info, sizeof(icon_info));
    ok(GetIconInfo(hicon2x2, &icon_info), "Expect GetIconInfo() success\n");
    ZeroMemory(&bm, sizeof(bm));
    ok(GetObjectW(icon_info.hbmColor, sizeof(bm), &bm), "Expect GetObjectW() success\n");
    ok(bm.bmWidth == 2 && bm.bmHeight == 2, "Expect bitmap size: %d,%d, got: %d,%d\n", 2, 2,
       bm.bmWidth, bm.bmHeight);
    DeleteObject(icon_info.hbmColor);
    DeleteObject(icon_info.hbmMask);

    hwnd = CreateWindowA(WC_BUTTONA, "test", default_style | BS_BITMAP, 0, 0, 100, 100, 0, 0,
                         0, 0);
    ok(hwnd != NULL, "Expect hwnd to be not NULL\n");
    /* Get image when image is not set */
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_GETIMAGE, IMAGE_BITMAP, 0);
    ok(hbmp == 0, "Expect hbmp == 0\n");
    /* Set image */
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp1x1);
    ok(hbmp == 0, "Expect hbmp == 0\n");
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_GETIMAGE, IMAGE_BITMAP, 0);
    ok(hbmp != 0, "Expect hbmp != 0\n");
    /* Set null resets image */
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_SETIMAGE, IMAGE_BITMAP, 0);
    ok(hbmp != 0, "Expect hbmp != 0\n");
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_GETIMAGE, IMAGE_BITMAP, 0);
    ok(hbmp == 0, "Expect hbmp == 0\n");
    DestroyWindow(hwnd);

    /* Set bitmap with BS_BITMAP */
    hwnd = CreateWindowA(WC_BUTTONA, "test", default_style | BS_BITMAP, 0, 0, 100, 100, 0, 0,
                         0, 0);
    ok(hwnd != NULL, "Expect hwnd to be not NULL\n");
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp1x1);
    ok(hbmp == 0, "Expect hbmp == 0\n");
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_GETIMAGE, IMAGE_BITMAP, 0);
    ok(hbmp != 0, "Expect hbmp != 0\n");
    ZeroMemory(&bm, sizeof(bm));
    ok(GetObjectW(hbmp, sizeof(bm), &bm), "Expect GetObjectW() success\n");
    ok(bm.bmWidth == 1 && bm.bmHeight == 1, "Expect bitmap size: %d,%d, got: %d,%d\n", 1, 1,
       bm.bmWidth, bm.bmHeight);
    DestroyWindow(hwnd);

    /* Set bitmap without BS_BITMAP */
    hwnd = CreateWindowA(WC_BUTTONA, "test", default_style, 0, 0, 100, 100, 0, 0, 0, 0);
    ok(hwnd != NULL, "Expect hwnd to be not NULL\n");
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp1x1);
    ok(hbmp == 0, "Expect hbmp == 0\n");
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_GETIMAGE, IMAGE_BITMAP, 0);
    if (hbmp == 0)
    {
        /* on xp or 2003*/
        win_skip("Show both image and text is not supported. Skip following tests.\n");
        DestroyWindow(hwnd);
        goto done;
    }
    ok(hbmp != 0, "Expect hbmp != 0\n");
    ZeroMemory(&bm, sizeof(bm));
    ok(GetObjectW(hbmp, sizeof(bm), &bm), "Expect GetObjectW() success\n");
    ok(bm.bmWidth == 1 && bm.bmHeight == 1, "Expect bitmap size: %d,%d, got: %d,%d\n", 1, 1,
       bm.bmWidth, bm.bmHeight);
    DestroyWindow(hwnd);

    /* Set icon with BS_ICON */
    hwnd = CreateWindowA(WC_BUTTONA, "test", default_style | BS_ICON, 0, 0, 100, 100, 0, 0, 0,
                         0);
    ok(hwnd != NULL, "Expect hwnd to be not NULL\n");
    hicon = (HICON)SendMessageA(hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon2x2);
    ok(hicon == 0, "Expect hicon == 0\n");
    hicon = (HICON)SendMessageA(hwnd, BM_GETIMAGE, IMAGE_ICON, 0);
    ok(hicon != 0, "Expect hicon != 0\n");
    ZeroMemory(&icon_info, sizeof(icon_info));
    ok(GetIconInfo(hicon, &icon_info), "Expect GetIconInfo() success\n");
    ZeroMemory(&bm, sizeof(bm));
    ok(GetObjectW(icon_info.hbmColor, sizeof(bm), &bm), "Expect GetObjectW() success\n");
    ok(bm.bmWidth == 2 && bm.bmHeight == 2, "Expect bitmap size: %d,%d, got: %d,%d\n", 2, 2,
       bm.bmWidth, bm.bmHeight);
    DeleteObject(icon_info.hbmColor);
    DeleteObject(icon_info.hbmMask);
    DestroyWindow(hwnd);

    /* Set icon without BS_ICON */
    hwnd = CreateWindowA(WC_BUTTONA, "test", default_style, 0, 0, 100, 100, 0, 0, 0, 0);
    ok(hwnd != NULL, "Expect hwnd to be not NULL\n");
    hicon = (HICON)SendMessageA(hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon2x2);
    ok(hicon == 0, "Expect hicon == 0\n");
    hicon = (HICON)SendMessageA(hwnd, BM_GETIMAGE, IMAGE_ICON, 0);
    ok(hicon != 0, "Expect hicon != 0\n");
    ZeroMemory(&icon_info, sizeof(icon_info));
    ok(GetIconInfo(hicon, &icon_info), "Expect GetIconInfo() success\n");
    ZeroMemory(&bm, sizeof(bm));
    ok(GetObjectW(icon_info.hbmColor, sizeof(bm), &bm), "Expect GetObjectW() success\n");
    ok(bm.bmWidth == 2 && bm.bmHeight == 2, "Expect bitmap size: %d,%d, got: %d,%d\n", 2, 2,
       bm.bmWidth, bm.bmHeight);
    DeleteObject(icon_info.hbmColor);
    DeleteObject(icon_info.hbmMask);
    DestroyWindow(hwnd);

    /* Set icon with BS_BITMAP */
    hwnd = CreateWindowA(WC_BUTTONA, "test", default_style | BS_BITMAP, 0, 0, 100, 100, 0, 0,
                         0, 0);
    ok(hwnd != NULL, "Expect hwnd to be not NULL\n");
    hicon = (HICON)SendMessageA(hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon2x2);
    ok(hicon == 0, "Expect hicon == 0\n");
    hicon = (HICON)SendMessageA(hwnd, BM_GETIMAGE, IMAGE_ICON, 0);
    ok(hicon != 0, "Expect hicon != 0\n");
    ZeroMemory(&icon_info, sizeof(icon_info));
    ok(GetIconInfo(hicon, &icon_info), "Expect GetIconInfo() success\n");
    ZeroMemory(&bm, sizeof(bm));
    ok(GetObjectW(icon_info.hbmColor, sizeof(bm), &bm), "Expect GetObjectW() success\n");
    ok(bm.bmWidth == 2 && bm.bmHeight == 2, "Expect bitmap size: %d,%d, got: %d,%d\n", 2, 2,
       bm.bmWidth, bm.bmHeight);
    DeleteObject(icon_info.hbmColor);
    DeleteObject(icon_info.hbmMask);
    DestroyWindow(hwnd);

    /* Set bitmap with BS_ICON */
    hwnd = CreateWindowA(WC_BUTTONA, "test", default_style | BS_ICON, 0, 0, 100, 100, 0, 0, 0,
                         0);
    ok(hwnd != NULL, "Expect hwnd to be not NULL\n");
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp1x1);
    ok(hbmp == 0, "Expect hbmp == 0\n");
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_GETIMAGE, IMAGE_BITMAP, 0);
    ok(hbmp != 0, "Expect hbmp != 0\n");
    ZeroMemory(&bm, sizeof(bm));
    ok(GetObjectW(hbmp, sizeof(bm), &bm), "Expect GetObjectW() success\n");
    ok(bm.bmWidth == 1 && bm.bmHeight == 1, "Expect bitmap size: %d,%d, got: %d,%d\n", 1, 1,
       bm.bmWidth, bm.bmHeight);
    DestroyWindow(hwnd);

    /* Set bitmap with BS_BITMAP and IMAGE_ICON*/
    hwnd = CreateWindowA(WC_BUTTONA, "test", default_style | BS_BITMAP, 0, 0, 100, 100, 0, 0,
                         0, 0);
    ok(hwnd != NULL, "Expect hwnd to be not NULL\n");
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hbmp1x1);
    ok(hbmp == 0, "Expect hbmp == 0\n");
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_GETIMAGE, IMAGE_ICON, 0);
    ok(hbmp != 0, "Expect hbmp != 0\n");
    ZeroMemory(&bm, sizeof(bm));
    ok(GetObjectW(hbmp, sizeof(bm), &bm), "Expect GetObjectW() success\n");
    ok(bm.bmWidth == 1 && bm.bmHeight == 1, "Expect bitmap size: %d,%d, got: %d,%d\n", 1, 1,
       bm.bmWidth, bm.bmHeight);
    DestroyWindow(hwnd);

    /* Set icon with BS_ICON and IMAGE_BITMAP */
    hwnd = CreateWindowA(WC_BUTTONA, "test", default_style | BS_ICON, 0, 0, 100, 100, 0, 0, 0,
                         0);
    ok(hwnd != NULL, "Expect hwnd to be not NULL\n");
    hicon = (HICON)SendMessageA(hwnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hicon2x2);
    ok(hicon == 0, "Expect hicon == 0\n");
    hicon = (HICON)SendMessageA(hwnd, BM_GETIMAGE, IMAGE_BITMAP, 0);
    ok(hicon != 0, "Expect hicon != 0\n");
    ZeroMemory(&icon_info, sizeof(icon_info));
    ok(GetIconInfo(hicon, &icon_info), "Expect GetIconInfo() success\n");
    ZeroMemory(&bm, sizeof(bm));
    ok(GetObjectW(icon_info.hbmColor, sizeof(BITMAP), &bm), "Expect GetObjectW() success\n");
    ok(bm.bmWidth == 2 && bm.bmHeight == 2, "Expect bitmap size: %d,%d, got: %d,%d\n", 2, 2,
       bm.bmWidth, bm.bmHeight);
    DeleteObject(icon_info.hbmColor);
    DeleteObject(icon_info.hbmMask);
    DestroyWindow(hwnd);

    /* Set bitmap with BS_ICON and IMAGE_ICON */
    hwnd = CreateWindowA(WC_BUTTONA, "test", default_style | BS_ICON, 0, 0, 100, 100, 0, 0, 0, 0);
    ok(hwnd != NULL, "Expect hwnd to be not NULL\n");
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hbmp1x1);
    ok(hbmp == 0, "Expect hbmp == 0\n");
    hbmp = (HBITMAP)SendMessageA(hwnd, BM_GETIMAGE, IMAGE_ICON, 0);
    ok(hbmp != 0, "Expect hbmp != 0\n");
    ZeroMemory(&bm, sizeof(bm));
    ok(GetObjectW(hbmp, sizeof(bm), &bm), "Expect GetObjectW() success\n");
    ok(bm.bmWidth == 1 && bm.bmHeight == 1, "Expect bitmap size: %d,%d, got: %d,%d\n", 1, 1,
       bm.bmWidth, bm.bmHeight);
    DestroyWindow(hwnd);

    /* Set icon with BS_BITMAP and IMAGE_BITMAP */
    hwnd = CreateWindowA(WC_BUTTONA, "test", default_style | BS_BITMAP, 0, 0, 100, 100, 0, 0, 0, 0);
    ok(hwnd != NULL, "Expect hwnd to be not NULL\n");
    hicon = (HICON)SendMessageA(hwnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hicon2x2);
    ok(hicon == 0, "Expect hicon == 0\n");
    hicon = (HICON)SendMessageA(hwnd, BM_GETIMAGE, IMAGE_BITMAP, 0);
    ok(hicon != 0, "Expect hicon != 0\n");
    ZeroMemory(&icon_info, sizeof(icon_info));
    ok(GetIconInfo(hicon, &icon_info), "Expect GetIconInfo() success\n");
    ZeroMemory(&bm, sizeof(bm));
    ok(GetObjectW(icon_info.hbmColor, sizeof(BITMAP), &bm), "Expect GetObjectW() success\n");
    ok(bm.bmWidth == 2 && bm.bmHeight == 2, "Expect bitmap size: %d,%d, got: %d,%d\n", 2, 2,
       bm.bmWidth, bm.bmHeight);
    DeleteObject(icon_info.hbmColor);
    DeleteObject(icon_info.hbmMask);
    DestroyWindow(hwnd);

done:
    DestroyIcon(hicon2x2);
    DeleteObject(hmask2x2);
    DeleteObject(hbmp2x2);
    DeleteObject(hbmp1x1);
    ReleaseDC(0, hdc);
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

static void test_bcm_splitinfo(HWND hwnd)
{
    UINT button = GetWindowLongA(hwnd, GWL_STYLE) & BS_TYPEMASK;
    int glyph_size = GetSystemMetrics(SM_CYMENUCHECK);
    int border_w = GetSystemMetrics(SM_CXEDGE) * 2;
    BUTTON_SPLITINFO info, dummy;
    HIMAGELIST img;
    BOOL ret;

    memset(&info, 0xCC, sizeof(info));
    info.mask = 0;
    memcpy(&dummy, &info, sizeof(info));

    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, (LPARAM)&info);
    if (ret != TRUE)
    {
        static BOOL once;
        if (!once)
            win_skip("BCM_GETSPLITINFO message is unavailable. Skipping related tests\n");  /* Pre-Vista */
        once = TRUE;
        return;
    }
    ok(!memcmp(&info, &dummy, sizeof(info)), "[%u] split info struct was changed with mask = 0\n", button);

    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, 0);
    ok(ret == FALSE, "[%u] expected FALSE, got %d\n", button, ret);
    ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, 0);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);

    info.mask = BCSIF_GLYPH | BCSIF_SIZE | BCSIF_STYLE;
    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ok(info.mask == (BCSIF_GLYPH | BCSIF_SIZE | BCSIF_STYLE), "[%u] wrong mask, got %u\n", button, info.mask);
    ok(info.himlGlyph == (HIMAGELIST)0x36, "[%u] expected 0x36 default glyph, got 0x%p\n", button, info.himlGlyph);
    ok(info.uSplitStyle == BCSS_STRETCH, "[%u] expected 0x%08x default style, got 0x%08x\n", button, BCSS_STRETCH, info.uSplitStyle);
    ok(info.size.cx == glyph_size, "[%u] expected %d default size.cx, got %ld\n", button, glyph_size, info.size.cx);
    ok(info.size.cy == 0, "[%u] expected 0 default size.cy, got %ld\n", button, info.size.cy);

    info.mask = BCSIF_SIZE;
    info.size.cx = glyph_size + 7;
    info.size.cy = 0;
    ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    info.size.cx = info.size.cy = 0xdeadbeef;
    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ok(info.mask == BCSIF_SIZE, "[%u] wrong mask, got %u\n", button, info.mask);
    ok(info.size.cx == glyph_size + 7, "[%u] expected %d, got %ld\n", button, glyph_size + 7, info.size.cx);
    ok(info.size.cy == 0, "[%u] expected 0, got %ld\n", button, info.size.cy);

    /* Invalid size.cx resets it to default glyph size, while size.cy is stored */
    info.size.cx = 0;
    info.size.cy = -20;
    ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    info.size.cx = info.size.cy = 0xdeadbeef;
    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ok(info.mask == BCSIF_SIZE, "[%u] wrong mask, got %u\n", button, info.mask);
    ok(info.size.cx == glyph_size, "[%u] expected %d, got %ld\n", button, glyph_size, info.size.cx);
    ok(info.size.cy == -20, "[%u] expected -20, got %ld\n", button, info.size.cy);

    info.size.cx = -glyph_size - 7;
    info.size.cy = -10;
    ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    info.size.cx = info.size.cy = 0xdeadbeef;
    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ok(info.mask == BCSIF_SIZE, "[%u] wrong mask, got %u\n", button, info.mask);
    ok(info.size.cx == glyph_size, "[%u] expected %d, got %ld\n", button, glyph_size, info.size.cx);
    ok(info.size.cy == -10, "[%u] expected -10, got %ld\n", button, info.size.cy);

    /* Set to a valid size other than glyph_size */
    info.mask = BCSIF_SIZE;
    info.size.cx = glyph_size + 7;
    info.size.cy = 11;
    ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    info.size.cx = info.size.cy = 0xdeadbeef;
    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ok(info.mask == BCSIF_SIZE, "[%u] wrong mask, got %u\n", button, info.mask);
    ok(info.size.cx == glyph_size + 7, "[%u] expected %d, got %ld\n", button, glyph_size + 7, info.size.cx);
    ok(info.size.cy == 11, "[%u] expected 11, got %ld\n", button, info.size.cy);

    /* Change the glyph, size.cx should be automatically adjusted and size.cy set to 0 */
    dummy.mask = BCSIF_GLYPH;
    dummy.himlGlyph = (HIMAGELIST)0x35;
    ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, (LPARAM)&dummy);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    info.mask = BCSIF_GLYPH | BCSIF_SIZE;
    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ok(info.mask == (BCSIF_GLYPH | BCSIF_SIZE), "[%u] wrong mask, got %u\n", button, info.mask);
    ok(info.himlGlyph == (HIMAGELIST)0x35, "[%u] expected 0x35, got %p\n", button, info.himlGlyph);
    ok(info.size.cx == glyph_size, "[%u] expected %d, got %ld\n", button, glyph_size, info.size.cx);
    ok(info.size.cy == 0, "[%u] expected 0, got %ld\n", button, info.size.cy);

    /* Unless the size is specified manually */
    dummy.mask = BCSIF_GLYPH | BCSIF_SIZE;
    dummy.himlGlyph = (HIMAGELIST)0x34;
    dummy.size.cx = glyph_size + 11;
    dummy.size.cy = 7;
    ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, (LPARAM)&dummy);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ok(info.mask == (BCSIF_GLYPH | BCSIF_SIZE), "[%u] wrong mask, got %u\n", button, info.mask);
    ok(info.himlGlyph == (HIMAGELIST)0x34, "[%u] expected 0x34, got %p\n", button, info.himlGlyph);
    ok(info.size.cx == glyph_size + 11, "[%u] expected %d, got %ld\n", button, glyph_size, info.size.cx);
    ok(info.size.cy == 7, "[%u] expected 7, got %ld\n", button, info.size.cy);

    /* Add the BCSS_IMAGE style manually with the wrong BCSIF_GLYPH mask, should treat it as invalid image */
    info.mask = BCSIF_GLYPH | BCSIF_STYLE;
    info.himlGlyph = (HIMAGELIST)0x37;
    info.uSplitStyle = BCSS_IMAGE;
    ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    info.mask |= BCSIF_SIZE;
    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ok(info.mask == (BCSIF_GLYPH | BCSIF_STYLE | BCSIF_SIZE), "[%u] wrong mask, got %u\n", button, info.mask);
    ok(info.himlGlyph == (HIMAGELIST)0x37, "[%u] expected 0x37, got %p\n", button, info.himlGlyph);
    ok(info.uSplitStyle == BCSS_IMAGE, "[%u] expected 0x%08x style, got 0x%08x\n", button, BCSS_IMAGE, info.uSplitStyle);
    ok(info.size.cx == border_w, "[%u] expected %d, got %ld\n", button, border_w, info.size.cx);
    ok(info.size.cy == 0, "[%u] expected 0, got %ld\n", button, info.size.cy);

    /* Change the size to prevent ambiguity */
    dummy.mask = BCSIF_SIZE;
    dummy.size.cx = glyph_size + 5;
    dummy.size.cy = 4;
    ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, (LPARAM)&dummy);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ok(info.mask == (BCSIF_GLYPH | BCSIF_STYLE | BCSIF_SIZE), "[%u] wrong mask, got %u\n", button, info.mask);
    ok(info.himlGlyph == (HIMAGELIST)0x37, "[%u] expected 0x37, got %p\n", button, info.himlGlyph);
    ok(info.uSplitStyle == BCSS_IMAGE, "[%u] expected 0x%08x style, got 0x%08x\n", button, BCSS_IMAGE, info.uSplitStyle);
    ok(info.size.cx == glyph_size + 5, "[%u] expected %d, got %ld\n", button, glyph_size + 5, info.size.cx);
    ok(info.size.cy == 4, "[%u] expected 4, got %ld\n", button, info.size.cy);

    /* Now remove the BCSS_IMAGE style manually with the wrong BCSIF_IMAGE mask */
    info.mask = BCSIF_IMAGE | BCSIF_STYLE;
    info.himlGlyph = (HIMAGELIST)0x35;
    info.uSplitStyle = BCSS_STRETCH;
    ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    info.mask |= BCSIF_SIZE;
    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ok(info.mask == (BCSIF_IMAGE | BCSIF_STYLE | BCSIF_SIZE), "[%u] wrong mask, got %u\n", button, info.mask);
    ok(info.himlGlyph == (HIMAGELIST)0x35, "[%u] expected 0x35, got %p\n", button, info.himlGlyph);
    ok(info.uSplitStyle == BCSS_STRETCH, "[%u] expected 0x%08x style, got 0x%08x\n", button, BCSS_STRETCH, info.uSplitStyle);
    ok(info.size.cx == glyph_size, "[%u] expected %d, got %ld\n", button, glyph_size, info.size.cx);
    ok(info.size.cy == 0, "[%u] expected 0, got %ld\n", button, info.size.cy);

    /* Add a proper valid image, the BCSS_IMAGE style should be set automatically */
    img = pImageList_Create(42, 33, ILC_COLOR, 1, 1);
    ok(img != NULL, "[%u] failed to create ImageList\n", button);
    info.mask = BCSIF_IMAGE;
    info.himlGlyph = img;
    ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    info.mask |= BCSIF_STYLE | BCSIF_SIZE;
    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ok(info.mask == (BCSIF_IMAGE | BCSIF_STYLE | BCSIF_SIZE), "[%u] wrong mask, got %u\n", button, info.mask);
    ok(info.himlGlyph == img, "[%u] expected %p, got %p\n", button, img, info.himlGlyph);
    ok(info.uSplitStyle == (BCSS_IMAGE | BCSS_STRETCH), "[%u] expected 0x%08x style, got 0x%08x\n", button, BCSS_IMAGE | BCSS_STRETCH, info.uSplitStyle);
    ok(info.size.cx == 42 + border_w, "[%u] expected %d, got %ld\n", button, 42 + border_w, info.size.cx);
    ok(info.size.cy == 0, "[%u] expected 0, got %ld\n", button, info.size.cy);
    pImageList_Destroy(img);
    dummy.mask = BCSIF_SIZE;
    dummy.size.cx = glyph_size + 5;
    dummy.size.cy = 4;
    ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, (LPARAM)&dummy);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);

    /* Change it to a glyph; when both specified, BCSIF_GLYPH takes priority */
    info.mask = BCSIF_GLYPH | BCSIF_IMAGE;
    info.himlGlyph = (HIMAGELIST)0x37;
    ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    info.mask |= BCSIF_STYLE | BCSIF_SIZE;
    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ok(info.mask == (BCSIF_GLYPH | BCSIF_IMAGE | BCSIF_STYLE | BCSIF_SIZE), "[%u] wrong mask, got %u\n", button, info.mask);
    ok(info.himlGlyph == (HIMAGELIST)0x37, "[%u] expected 0x37, got %p\n", button, info.himlGlyph);
    ok(info.uSplitStyle == BCSS_STRETCH, "[%u] expected 0x%08x style, got 0x%08x\n", button, BCSS_STRETCH, info.uSplitStyle);
    ok(info.size.cx == glyph_size, "[%u] expected %d, got %ld\n", button, glyph_size, info.size.cx);
    ok(info.size.cy == 0, "[%u] expected 0, got %ld\n", button, info.size.cy);

    /* Try a NULL image */
    info.mask = BCSIF_IMAGE;
    info.himlGlyph = NULL;
    ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    info.mask |= BCSIF_STYLE | BCSIF_SIZE;
    ret = SendMessageA(hwnd, BCM_GETSPLITINFO, 0, (LPARAM)&info);
    ok(ret == TRUE, "[%u] expected TRUE, got %d\n", button, ret);
    ok(info.mask == (BCSIF_IMAGE | BCSIF_STYLE | BCSIF_SIZE), "[%u] wrong mask, got %u\n", button, info.mask);
    ok(info.himlGlyph == NULL, "[%u] expected NULL, got %p\n", button, info.himlGlyph);
    ok(info.uSplitStyle == (BCSS_IMAGE | BCSS_STRETCH), "[%u] expected 0x%08x style, got 0x%08x\n", button, BCSS_IMAGE | BCSS_STRETCH, info.uSplitStyle);
    ok(info.size.cx == border_w, "[%u] expected %d, got %ld\n", button, border_w, info.size.cx);
    ok(info.size.cy == 0, "[%u] expected 0, got %ld\n", button, info.size.cy);
}

static void test_button_data(void)
{
    static const DWORD styles[] =
    {
        BS_PUSHBUTTON,
        BS_DEFPUSHBUTTON,
        BS_CHECKBOX,
        BS_AUTOCHECKBOX,
        BS_RADIOBUTTON,
        BS_3STATE,
        BS_AUTO3STATE,
        BS_GROUPBOX,
        BS_USERBUTTON,
        BS_AUTORADIOBUTTON,
        BS_OWNERDRAW,
        BS_SPLITBUTTON,
        BS_DEFSPLITBUTTON,
        BS_COMMANDLINK,
        BS_DEFCOMMANDLINK,
    };

    struct button_desc
    {
        HWND self;
        HWND parent;
        LONG style;
    };
    unsigned int i;
    HWND parent;

    parent = CreateWindowExA(0, "TestParentClass", "Test parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             100, 100, 200, 200, 0, 0, 0, NULL);
    ok(parent != 0, "Failed to create parent window\n");

    for (i = 0; i < ARRAY_SIZE(styles); i++)
    {
        struct button_desc *desc;
        HWND hwnd;

        hwnd = create_button(styles[i], parent);
        ok(hwnd != NULL, "Failed to create a button.\n");

        desc = (void *)GetWindowLongPtrA(hwnd, 0);
        ok(desc != NULL, "Expected window data.\n");

        if (desc)
        {
            ok(desc->self == hwnd, "Unexpected 'self' field.\n");
            ok(desc->parent == parent, "Unexpected 'parent' field.\n");
            ok(desc->style == (WS_CHILD | BS_NOTIFY | styles[i]), "Unexpected 'style' field.\n");
        }

        /* Data set and retrieved by these messages is valid for all buttons */
        test_bcm_splitinfo(hwnd);

        DestroyWindow(hwnd);
    }

    DestroyWindow(parent);
}

static void test_get_set_imagelist(void)
{
    HWND hwnd;
    HIMAGELIST himl;
    BUTTON_IMAGELIST biml = {0};
    HDC hdc;
    HBITMAP hbmp;
    INT width = 16;
    INT height = 16;
    INT index;
    DWORD type;
    BOOL ret;

    hdc = GetDC(0);
    hbmp = CreateCompatibleBitmap(hdc, width, height);
    ok(hbmp != NULL, "Expect hbmp not null\n");

    himl = pImageList_Create(width, height, ILC_COLOR, 1, 0);
    ok(himl != NULL, "Expect himl not null\n");
    index = pImageList_Add(himl, hbmp, NULL);
    ok(index == 0, "Expect index == 0\n");
    DeleteObject(hbmp);
    ReleaseDC(0, hdc);

    for (type = BS_PUSHBUTTON; type <= BS_DEFCOMMANDLINK; type++)
    {
        hwnd = create_button(type, NULL);
        ok(hwnd != NULL, "Expect hwnd not null\n");

        /* Get imagelist when imagelist is unset yet */
        ret = SendMessageA(hwnd, BCM_GETIMAGELIST, 0, (LPARAM)&biml);
        ok(ret, "Expect BCM_GETIMAGELIST return true\n");
        ok(biml.himl == 0 && IsRectEmpty(&biml.margin) && biml.uAlign == 0,
           "Expect BUTTON_IMAGELIST is empty\n");

        /* Set imagelist with himl null */
        biml.himl = 0;
        biml.uAlign = BUTTON_IMAGELIST_ALIGN_CENTER;
        ret = SendMessageA(hwnd, BCM_SETIMAGELIST, 0, (LPARAM)&biml);
        ok(ret || broken(!ret), /* xp or 2003 */
           "Expect BCM_SETIMAGELIST return true\n");

        /* Set imagelist with uAlign invalid */
        biml.himl = himl;
        biml.uAlign = -1;
        ret = SendMessageA(hwnd, BCM_SETIMAGELIST, 0, (LPARAM)&biml);
        ok(ret, "Expect BCM_SETIMAGELIST return true\n");

        /* Successful get and set imagelist */
        biml.himl = himl;
        biml.uAlign = BUTTON_IMAGELIST_ALIGN_CENTER;
        ret = SendMessageA(hwnd, BCM_SETIMAGELIST, 0, (LPARAM)&biml);
        ok(ret, "Expect BCM_SETIMAGELIST return true\n");
        ret = SendMessageA(hwnd, BCM_GETIMAGELIST, 0, (LPARAM)&biml);
        ok(ret, "Expect BCM_GETIMAGELIST return true\n");
        ok(biml.himl == himl, "Expect himl to be same\n");
        ok(biml.uAlign == BUTTON_IMAGELIST_ALIGN_CENTER, "Expect uAlign to be %x\n",
           BUTTON_IMAGELIST_ALIGN_CENTER);

        /* BCM_SETIMAGELIST null pointer handling */
        ret = SendMessageA(hwnd, BCM_SETIMAGELIST, 0, 0);
        ok(!ret, "Expect BCM_SETIMAGELIST return false\n");
        ret = SendMessageA(hwnd, BCM_GETIMAGELIST, 0, (LPARAM)&biml);
        ok(ret, "Expect BCM_GETIMAGELIST return true\n");
        ok(biml.himl == himl, "Expect himl to be same\n");

        /* BCM_GETIMAGELIST null pointer handling */
        biml.himl = himl;
        biml.uAlign = BUTTON_IMAGELIST_ALIGN_CENTER;
        ret = SendMessageA(hwnd, BCM_SETIMAGELIST, 0, (LPARAM)&biml);
        ok(ret, "Expect BCM_SETIMAGELIST return true\n");
        ret = SendMessageA(hwnd, BCM_GETIMAGELIST, 0, 0);
        ok(!ret, "Expect BCM_GETIMAGELIST return false\n");

        DestroyWindow(hwnd);
    }

    pImageList_Destroy(himl);
}

static void test_get_set_textmargin(void)
{
    HWND hwnd;
    RECT margin_in;
    RECT margin_out;
    BOOL ret;
    DWORD type;

    SetRect(&margin_in, 2, 1, 3, 4);
    for (type = BS_PUSHBUTTON; type <= BS_DEFCOMMANDLINK; type++)
    {
        hwnd = create_button(type, NULL);
        ok(hwnd != NULL, "Expect hwnd not null\n");

        /* Get text margin when it is unset */
        ret = SendMessageA(hwnd, BCM_GETTEXTMARGIN, 0, (LPARAM)&margin_out);
        ok(ret, "Expect ret to be true\n");
        ok(IsRectEmpty(&margin_out), "Expect margin empty\n");

        /* Successful get and set text margin */
        ret = SendMessageA(hwnd, BCM_SETTEXTMARGIN, 0, (LPARAM)&margin_in);
        ok(ret, "Expect ret to be true\n");
        SetRectEmpty(&margin_out);
        ret = SendMessageA(hwnd, BCM_GETTEXTMARGIN, 0, (LPARAM)&margin_out);
        ok(ret, "Expect ret to be true\n");
        ok(EqualRect(&margin_in, &margin_out), "Expect margins to be equal\n");

        /* BCM_SETTEXTMARGIN null pointer handling */
        ret = SendMessageA(hwnd, BCM_SETTEXTMARGIN, 0, 0);
        ok(!ret, "Expect ret to be false\n");
        SetRectEmpty(&margin_out);
        ret = SendMessageA(hwnd, BCM_GETTEXTMARGIN, 0, (LPARAM)&margin_out);
        ok(ret, "Expect ret to be true\n");
        ok(EqualRect(&margin_in, &margin_out), "Expect margins to be equal\n");

        /* BCM_GETTEXTMARGIN null pointer handling */
        ret = SendMessageA(hwnd, BCM_SETTEXTMARGIN, 0, (LPARAM)&margin_in);
        ok(ret, "Expect ret to be true\n");
        ret = SendMessageA(hwnd, BCM_GETTEXTMARGIN, 0, 0);
        ok(!ret, "Expect ret to be true\n");

        DestroyWindow(hwnd);
    }
}

static void test_state(void)
{
    HWND hwnd;
    DWORD type;
    LONG state;

    /* Initial button state */
    for (type = BS_PUSHBUTTON; type <= BS_DEFCOMMANDLINK; type++)
    {
        hwnd = create_button(type, NULL);
        state = SendMessageA(hwnd, BM_GETSTATE, 0, 0);
        ok(state == BST_UNCHECKED, "Expect state 0x%08x, got 0x%08lx\n", BST_UNCHECKED, state);
        DestroyWindow(hwnd);
    }
}

static void test_bcm_get_ideal_size(void)
{
    static const char *button_text2 = "WWWW\nWWWW";
    static const char *button_text = "WWWW";
    static const DWORD imagelist_aligns[] = {BUTTON_IMAGELIST_ALIGN_LEFT, BUTTON_IMAGELIST_ALIGN_RIGHT,
                                             BUTTON_IMAGELIST_ALIGN_TOP, BUTTON_IMAGELIST_ALIGN_BOTTOM,
                                             BUTTON_IMAGELIST_ALIGN_CENTER};
    static const DWORD aligns[] = {0,         BS_TOP,     BS_LEFT,        BS_RIGHT,   BS_BOTTOM,
                                   BS_CENTER, BS_VCENTER, BS_RIGHTBUTTON, WS_EX_RIGHT};
    DWORD default_style = WS_TABSTOP | WS_POPUP | WS_VISIBLE;
    const LONG client_width = 400, client_height = 200, extra_width = 123, large_height = 500;
    struct
    {
        DWORD style;
        LONG extra_width;
    } pushtype[] =
    {
        { BS_PUSHBUTTON, 0 },
        { BS_DEFPUSHBUTTON, 0 },
        { BS_SPLITBUTTON, extra_width * 2 + GetSystemMetrics(SM_CXEDGE) },
        { BS_DEFSPLITBUTTON, extra_width * 2 + GetSystemMetrics(SM_CXEDGE) }
    };
    LONG image_width = 48, height = 48, line_count, text_width;
    HFONT hfont, prev_font;
    DWORD style, type;
    BOOL ret;
    HWND hwnd;
    HDC hdc;
    LOGFONTA lf;
    TEXTMETRICA tm;
    SIZE size;
    HBITMAP hmask, hbmp;
    ICONINFO icon_info;
    HICON hicon;
    HIMAGELIST himl;
    BUTTON_IMAGELIST biml = {0};
    RECT rect;
    INT i, j, k;

    /* Check for NULL pointer handling */
    hwnd = CreateWindowA(WC_BUTTONA, button_text, BS_PUSHBUTTON | default_style, 0, 0, client_width, client_height,
        NULL, NULL, 0, NULL);
    ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, 0);
    ok(!ret, "Expect BCM_GETIDEALSIZE message to return false.\n");

    /* Set font so that the test is consistent on Wine and Windows */
    ZeroMemory(&lf, sizeof(lf));
    lf.lfWeight = FW_NORMAL;
    lf.lfHeight = 20;
    lstrcpyA(lf.lfFaceName, "Tahoma");
    hfont = CreateFontIndirectA(&lf);
    ok(hfont != NULL, "Failed to create test font.\n");

    /* Get tmHeight */
    hdc = GetDC(hwnd);
    prev_font = SelectObject(hdc, hfont);
    GetTextMetricsA(hdc, &tm);
    SelectObject(hdc, prev_font);
    DrawTextA(hdc, button_text, -1, &rect, DT_CALCRECT);
    text_width = rect.right - rect.left;
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);

    /* XP and 2003 doesn't support command links, getting ideal size with button having only text returns client size on these platforms. */
    hwnd = CreateWindowA(WC_BUTTONA, button_text, BS_DEFCOMMANDLINK | default_style, 0, 0, client_width, client_height, NULL,
                         NULL, 0, NULL);
    ok(hwnd != NULL, "Expect hwnd not NULL\n");
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
    ZeroMemory(&size, sizeof(size));
    ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
    ok(ret, "Expect BCM_GETIDEALSIZE message to return true\n");
    if (size.cx == client_width && size.cy == client_height)
    {
        /* on XP and 2003, buttons with image are not supported */
        win_skip("Skipping further tests on XP and 2003\n");
        return;
    }

    /* Tests for image placements */
    /* Prepare bitmap */
    hdc = GetDC(0);
    hmask = CreateCompatibleBitmap(hdc, image_width, height);
    hbmp = CreateCompatibleBitmap(hdc, image_width, height);
    himl = pImageList_Create(image_width, height, ILC_COLOR, 1, 1);
    pImageList_Add(himl, hbmp, 0);

#define set_split_info(hwnd) do { \
    BUTTON_SPLITINFO _info; \
    int _ret; \
    _info.mask = BCSIF_SIZE; \
    _info.size.cx = extra_width; \
    _info.size.cy = large_height; \
    _ret = SendMessageA(hwnd, BCM_SETSPLITINFO, 0, (LPARAM)&_info); \
    ok(_ret == TRUE, "Expected BCM_SETSPLITINFO message to return true\n"); \
} while (0)

    for (k = 0; k < ARRAY_SIZE(pushtype); k++)
    {
        /* Only bitmap for push button, ideal size should be enough for image and text */
        hwnd = CreateWindowA(WC_BUTTONA, button_text, pushtype[k].style | BS_BITMAP | default_style, 0, 0, client_width,
                             client_height, NULL, NULL, 0, NULL);
        ok(hwnd != NULL, "Expect hwnd not NULL\n");
        set_split_info(hwnd);
        SendMessageA(hwnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbmp);
        SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
        ZeroMemory(&size, sizeof(size));
        ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
        ok(ret, "Expect BCM_GETIDEALSIZE message to return true\n");
        /* Ideal size contains text rect even show bitmap only */
        ok(size.cx >= image_width + text_width + pushtype[k].extra_width && size.cy >= max(height, tm.tmHeight),
                "Expect ideal cx %ld >= %ld and ideal cy %ld >= %ld\n", size.cx,
                image_width + text_width + pushtype[k].extra_width, size.cy, max(height, tm.tmHeight));
        ok(size.cy < large_height, "Expect ideal cy %ld < %ld\n", size.cy, large_height);
        DestroyWindow(hwnd);

        /* Image alignments when button has bitmap and text*/
        for (i = 0; i < ARRAY_SIZE(aligns); i++)
            for (j = 0; j < ARRAY_SIZE(aligns); j++)
            {
                style = pushtype[k].style | default_style | aligns[i] | aligns[j];
                hwnd = CreateWindowA(WC_BUTTONA, button_text, style, 0, 0, client_width, client_height, NULL, NULL, 0, NULL);
                ok(hwnd != NULL, "Expect hwnd not NULL\n");
                set_split_info(hwnd);
                SendMessageA(hwnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbmp);
                SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
                ZeroMemory(&size, sizeof(size));
                ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
                ok(ret, "Expect BCM_GETIDEALSIZE message to return true\n");
                if (!(style & (BS_CENTER | BS_VCENTER)) || ((style & BS_CENTER) && (style & BS_CENTER) != BS_CENTER)
                    || !(style & BS_VCENTER) || (style & BS_VCENTER) == BS_VCENTER)
                    ok(size.cx >= image_width + text_width + pushtype[k].extra_width && size.cy >= max(height, tm.tmHeight),
                       "Style: 0x%08lx expect ideal cx %ld >= %ld and ideal cy %ld >= %ld\n", style, size.cx,
                       image_width + text_width + pushtype[k].extra_width, size.cy, max(height, tm.tmHeight));
                else
                    ok(size.cx >= max(text_width, height) + pushtype[k].extra_width && size.cy >= height + tm.tmHeight,
                       "Style: 0x%08lx expect ideal cx %ld >= %ld and ideal cy %ld >= %ld\n", style, size.cx,
                       max(text_width, height) + pushtype[k].extra_width, size.cy, height + tm.tmHeight);
                ok(size.cy < large_height, "Expect ideal cy %ld < %ld\n", size.cy, large_height);
                DestroyWindow(hwnd);
            }

        /* Image list alignments */
        biml.himl = himl;
        for (i = 0; i < ARRAY_SIZE(imagelist_aligns); i++)
        {
            biml.uAlign = imagelist_aligns[i];
            hwnd = CreateWindowA(WC_BUTTONA, button_text, pushtype[k].style | default_style, 0, 0, client_width,
                client_height, NULL, NULL, 0, NULL);
            ok(hwnd != NULL, "Expect hwnd not NULL\n");
            set_split_info(hwnd);
            SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
            SendMessageA(hwnd, BCM_SETIMAGELIST, 0, (LPARAM)&biml);
            ZeroMemory(&size, sizeof(size));
            ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
            ok(ret, "Expect BCM_GETIDEALSIZE message to return true\n");
            if (biml.uAlign == BUTTON_IMAGELIST_ALIGN_TOP || biml.uAlign == BUTTON_IMAGELIST_ALIGN_BOTTOM)
                ok(size.cx >= max(text_width, height) + pushtype[k].extra_width && size.cy >= height + tm.tmHeight,
                   "Align:%d expect ideal cx %ld >= %ld and ideal cy %ld >= %ld\n", biml.uAlign, size.cx,
                   max(text_width, height) + pushtype[k].extra_width, size.cy, height + tm.tmHeight);
            else if (biml.uAlign == BUTTON_IMAGELIST_ALIGN_LEFT || biml.uAlign == BUTTON_IMAGELIST_ALIGN_RIGHT)
                ok(size.cx >= image_width + text_width + pushtype[k].extra_width && size.cy >= max(height, tm.tmHeight),
                   "Align:%d expect ideal cx %ld >= %ld and ideal cy %ld >= %ld\n", biml.uAlign, size.cx,
                   image_width + text_width + pushtype[k].extra_width, size.cy, max(height, tm.tmHeight));
            else
                ok(size.cx >= image_width + pushtype[k].extra_width && size.cy >= height,
                   "Align:%d expect ideal cx %ld >= %ld and ideal cy %ld >= %ld\n",
                   biml.uAlign, size.cx, image_width + pushtype[k].extra_width, size.cy, height);
            ok(size.cy < large_height, "Expect ideal cy %ld < %ld\n", size.cy, large_height);
            DestroyWindow(hwnd);
        }

        /* Icon as image */
        /* Create icon from bitmap */
        ZeroMemory(&icon_info, sizeof(icon_info));
        icon_info.fIcon = TRUE;
        icon_info.hbmMask = hmask;
        icon_info.hbmColor = hbmp;
        hicon = CreateIconIndirect(&icon_info);

        /* Only icon, ideal size should be enough for image and text */
        hwnd = CreateWindowA(WC_BUTTONA, button_text, pushtype[k].style | BS_ICON | default_style, 0, 0, client_width,
                             client_height, NULL, NULL, 0, NULL);
        ok(hwnd != NULL, "Expect hwnd not NULL\n");
        set_split_info(hwnd);
        SendMessageA(hwnd, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hicon);
        SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
        ZeroMemory(&size, sizeof(size));
        ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
        ok(ret, "Expect BCM_GETIDEALSIZE message to return true\n");
        /* Ideal size contains text rect even show icons only */
        ok(size.cx >= image_width + text_width + pushtype[k].extra_width && size.cy >= max(height, tm.tmHeight),
           "Expect ideal cx %ld >= %ld and ideal cy %ld >= %ld\n", size.cx,
           image_width + text_width + pushtype[k].extra_width, size.cy, max(height, tm.tmHeight));
        ok(size.cy < large_height, "Expect ideal cy %ld < %ld\n", size.cy, large_height);
        DestroyWindow(hwnd);

        /* Show icon and text */
        hwnd = CreateWindowA(WC_BUTTONA, button_text, pushtype[k].style | default_style, 0, 0, client_width,
            client_height, NULL, NULL, 0, NULL);
        ok(hwnd != NULL, "Expect hwnd not NULL\n");
        set_split_info(hwnd);
        SendMessageA(hwnd, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hicon);
        SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
        ZeroMemory(&size, sizeof(size));
        ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
        ok(ret, "Expect BCM_GETIDEALSIZE message to return true\n");
        ok(size.cx >= image_width + text_width + pushtype[k].extra_width && size.cy >= max(height, tm.tmHeight),
           "Expect ideal cx %ld >= %ld and ideal cy %ld >= %ld\n", size.cx,
           image_width + text_width + pushtype[k].extra_width, size.cy, max(height, tm.tmHeight));
        ok(size.cy < large_height, "Expect ideal cy %ld < %ld\n", size.cy, large_height);
        DestroyWindow(hwnd);
        DestroyIcon(hicon);
    }

#undef set_split_info

    /* Checkbox */
    /* Both bitmap and text for checkbox, ideal size is only enough for text because it doesn't support image(but not image list)*/
    hwnd = CreateWindowA(WC_BUTTONA, button_text, BS_AUTOCHECKBOX | default_style, 0, 0, client_width, client_height,
        NULL, NULL, 0, NULL);
    ok(hwnd != NULL, "Expect hwnd not NULL\n");
    SendMessageA(hwnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbmp);
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
    ZeroMemory(&size, sizeof(size));
    ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
    ok(ret, "Expect BCM_GETIDEALSIZE message to return true\n");
    ok((size.cx <= image_width + text_width && size.cx >= text_width && size.cy <= max(height, tm.tmHeight)
        && size.cy >= tm.tmHeight),
       "Expect ideal cx %ld within range (%ld, %ld ) and ideal cy %ld within range (%ld, %ld )\n", size.cx,
       text_width, image_width + text_width, size.cy, tm.tmHeight, max(height, tm.tmHeight));
    DestroyWindow(hwnd);

    /* Both image list and text for checkbox, ideal size should have enough for image list and text */
    biml.uAlign = BUTTON_IMAGELIST_ALIGN_LEFT;
    hwnd = CreateWindowA(WC_BUTTONA, button_text, BS_AUTOCHECKBOX | BS_BITMAP | default_style, 0, 0, client_width,
                         client_height, NULL, NULL, 0, NULL);
    ok(hwnd != NULL, "Expect hwnd not NULL\n");
    SendMessageA(hwnd, BCM_SETIMAGELIST, 0, (LPARAM)&biml);
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
    ZeroMemory(&size, sizeof(size));
    ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
    ok(ret, "Expect BCM_GETIDEALSIZE message to return true\n");
    ok((size.cx >= image_width + text_width && size.cy >= max(height, tm.tmHeight)),
       "Expect ideal cx %ld >= %ld and ideal cy %ld >= %ld\n", size.cx, image_width + text_width, size.cy,
       max(height, tm.tmHeight));
    DestroyWindow(hwnd);

    /* Only bitmap for checkbox, ideal size should have enough for image and text */
    hwnd = CreateWindowA(WC_BUTTONA, button_text, BS_AUTOCHECKBOX | BS_BITMAP | default_style, 0, 0, client_width,
                         client_height, NULL, NULL, 0, NULL);
    ok(hwnd != NULL, "Expect hwnd not NULL\n");
    SendMessageA(hwnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbmp);
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
    ZeroMemory(&size, sizeof(size));
    ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
    ok(ret, "Expect BCM_GETIDEALSIZE message to return true\n");
    ok((size.cx >= image_width + text_width && size.cy >= max(height, tm.tmHeight)),
       "Expect ideal cx %ld >= %ld and ideal cy %ld >= %ld\n", size.cx, image_width + text_width, size.cy,
       max(height, tm.tmHeight));
    DestroyWindow(hwnd);

    /* Test button with only text */
    /* No text */
    for (type = BS_PUSHBUTTON; type <= BS_DEFCOMMANDLINK; type++)
    {
        style = type | default_style;
        hwnd = CreateWindowA(WC_BUTTONA, "", style, 0, 0, client_width, client_height, NULL, NULL, 0, NULL);
        ok(hwnd != NULL, "Expect hwnd not NULL\n");
        SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);

        ZeroMemory(&size, sizeof(size));
        ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
        ok(ret, "Expect BCM_GETIDEALSIZE message to return true\n");

        if (type == BS_COMMANDLINK || type == BS_DEFCOMMANDLINK)
        {
            ok((size.cx == 0 && size.cy > 0), "Style 0x%08lx expect ideal cx %ld == %d and ideal cy %ld > %d\n",
               style, size.cx, 0, size.cy, 0);
        }
        else
        {
            ok(size.cx == client_width && size.cy == client_height,
               "Style 0x%08lx expect size.cx == %ld and size.cy == %ld, got size.cx: %ld size.cy: %ld\n", style,
               client_width, client_height, size.cx, size.cy);
        }
        DestroyWindow(hwnd);
    }

    /* Single line and multiple lines text */
    for (line_count = 1; line_count <= 2; line_count++)
    {
        for (type = BS_PUSHBUTTON; type <= BS_DEFCOMMANDLINK; type++)
        {
            style = line_count > 1 ? type | BS_MULTILINE : type;
            style |= default_style;

            hwnd = CreateWindowA(WC_BUTTONA, (line_count == 2 ? button_text2 : button_text), style, 0, 0, client_width,
                                 client_height, NULL, NULL, 0, NULL);
            ok(hwnd != NULL, "Expect hwnd not NULL\n");
            SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
            ZeroMemory(&size, sizeof(size));
            ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
            ok(ret, "Expect BCM_GETIDEALSIZE message to return true\n");

            if (type == BS_3STATE || type == BS_AUTO3STATE || type == BS_GROUPBOX || type == BS_PUSHBOX
                || type == BS_OWNERDRAW)
            {
                ok(size.cx == client_width && size.cy == client_height,
                   "Style 0x%08lx expect ideal size (%ld,%ld), got (%ld,%ld)\n", style, client_width, client_height, size.cx,
                   size.cy);
            }
            else if (type == BS_COMMANDLINK || type == BS_DEFCOMMANDLINK)
            {
                ok((size.cx == 0 && size.cy > 0),
                   "Style 0x%08lx expect ideal cx %ld == %d and ideal cy %ld > %d\n", style, size.cx, 0,
                   size.cy, 0);
            }
            else
            {
                height = line_count == 2 ? 2 * tm.tmHeight : tm.tmHeight;
                ok(size.cx >= 0 && size.cy >= height, "Style 0x%08lx expect ideal cx %ld >= 0 and ideal cy %ld >= %ld\n",
                    style, size.cx, size.cy, height);
            }
            DestroyWindow(hwnd);
        }
    }

    /* Command Link with note */
    hwnd = CreateWindowA(WC_BUTTONA, "a", style, 0, 0, client_width, client_height, NULL, NULL, 0, NULL);
    ok(hwnd != NULL, "Expected hwnd not NULL\n");
    SendMessageA(hwnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);
    ret = SendMessageA(hwnd, BCM_SETNOTE, 0, (LPARAM)L"W");
    ok(ret == TRUE, "Expected BCM_SETNOTE to return true\n");
    size.cx = 13;
    size.cy = 0;
    ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
    ok(ret == TRUE, "Expected BCM_GETIDEALSIZE message to return true\n");
    ok(size.cx == 13 && size.cy > 0, "Expected ideal cx %ld == %d and ideal cy %ld > %d\n", size.cx, 13, size.cy, 0);
    height = size.cy;
    size.cx = 32767;
    size.cy = 7;
    ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
    ok(ret == TRUE, "Expected BCM_GETIDEALSIZE message to return true\n");
    ok(size.cx < 32767, "Expected ideal cx to have been adjusted\n");
    ok(size.cx > image_width && size.cy == height, "Expected ideal cx %ld > %ld and ideal cy %ld == %ld\n", size.cx, image_width, size.cy, height);

    /* Try longer note without word breaks, shouldn't extend height because no word splitting */
    ret = SendMessageA(hwnd, BCM_SETNOTE, 0, (LPARAM)L"WWWWWWWWWWWWWWWW");
    ok(ret == TRUE, "Expected BCM_SETNOTE to return true\n");
    k = size.cx;
    size.cy = 0;
    ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
    ok(ret == TRUE, "Expected BCM_GETIDEALSIZE message to return true\n");
    ok(size.cx == k && size.cy == height, "Expected ideal cx %ld == %d and ideal cy %ld == %ld\n", size.cx, k, size.cy, height);

    /* Now let it extend the width */
    size.cx = 32767;
    size.cy = 0;
    ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
    ok(ret == TRUE, "Expected BCM_GETIDEALSIZE message to return true\n");
    ok(size.cx > k && size.cy == height, "Expected ideal cx %ld > %d and ideal cy %ld == %ld\n", size.cx, k, size.cy, height);

    /* Use a very long note with words and the same width, should extend the height due to word wrap */
    ret = SendMessageA(hwnd, BCM_SETNOTE, 0, (LPARAM)L"This is a long note for the button with many words, which should "
            "be overall longer than the text (given the smaller font) and thus wrap.");
    ok(ret == TRUE, "Expected BCM_SETNOTE to return true\n");
    k = size.cx;
    ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
    ok(ret == TRUE, "Expected BCM_GETIDEALSIZE message to return true\n");
    ok(size.cx <= k && size.cy > height, "Expected ideal cx %ld <= %d and ideal cy %ld > %ld\n", size.cx, k, size.cy, height);

    /* Now try the wordy note with a width smaller than the image itself, which prevents wrapping */
    size.cx = 13;
    ret = SendMessageA(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&size);
    ok(ret == TRUE, "Expected BCM_GETIDEALSIZE message to return true\n");
    ok(size.cx == 13 && size.cy == height, "Expected ideal cx %ld == %d and ideal cy %ld == %ld\n", size.cx, 13, size.cy, height);
    DestroyWindow(hwnd);


    pImageList_Destroy(himl);
    DeleteObject(hbmp);
    DeleteObject(hmask);
    ReleaseDC(0, hdc);
    DeleteObject(hfont);
}

static void test_style(void)
{
    DWORD type, expected_type;
    HWND button;
    LRESULT ret;
    DWORD i, j;

    for (i = BS_PUSHBUTTON; i <= BS_DEFCOMMANDLINK; ++i)
    {
        button = CreateWindowA(WC_BUTTONA, "test", i, 0, 0, 50, 50, NULL, 0, 0, NULL);
        ok(button != NULL, "Expected button not null.\n");

        type = GetWindowLongW(button, GWL_STYLE) & BS_TYPEMASK;
        expected_type = (i == BS_USERBUTTON ? BS_PUSHBUTTON : i);
        ok(type == expected_type, "Expected type %#lx, got %#lx.\n", expected_type, type);

        for (j = BS_PUSHBUTTON; j <= BS_DEFCOMMANDLINK; ++j)
        {
            ret = SendMessageA(button, BM_SETSTYLE, j, FALSE);
            ok(ret == 0, "Expected %#x, got %#Ix.\n", 0, ret);

            type = GetWindowLongW(button, GWL_STYLE) & BS_TYPEMASK;
            if (i >= BS_SPLITBUTTON && j <= BS_DEFPUSHBUTTON)
                expected_type = (i & ~BS_DEFPUSHBUTTON) | j;
            else
                expected_type = j;

            ok(type == expected_type || broken(type == j), /* XP */
                    "Original type %#lx, expected new type %#lx, got %#lx.\n", i, expected_type, type);
        }
        DestroyWindow(button);
    }
}

static void test_visual(void)
{
    HBITMAP mem_bitmap1, mem_bitmap2;
    HDC mem_dc1, mem_dc2, button_dc;
    HWND button, parent;
    int width, height;
    DWORD type;
    RECT rect;

    parent = CreateWindowA("TestParentClass", "Test parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100,
            100, 200, 200, 0, 0, 0, NULL);
    ok(!!parent, "Failed to create the parent window.\n");
    for (type = BS_PUSHBUTTON; type <= BS_DEFCOMMANDLINK; ++type)
    {
        /* Use WS_DISABLED to avoid glowing animations on Vista and Win7 */
        button = create_button(type | WS_VISIBLE | WS_DISABLED, parent);
        flush_events();

        button_dc = GetDC(button);
        GetClientRect(button, &rect);
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;

        mem_dc1 = CreateCompatibleDC(button_dc);
        mem_bitmap1 = CreateCompatibleBitmap(button_dc, width, height);
        SelectObject(mem_dc1, mem_bitmap1);
        BitBlt(mem_dc1, 0, 0, width, height, button_dc, 0, 0, SRCCOPY);

        /* Send WM_SETTEXT with the same window name */
        SendMessageA(button, WM_SETTEXT, 0, (LPARAM)"test");
        flush_events();

        mem_dc2 = CreateCompatibleDC(button_dc);
        mem_bitmap2 = CreateCompatibleBitmap(button_dc, width, height);
        SelectObject(mem_dc2, mem_bitmap2);
        BitBlt(mem_dc2, 0, 0, width, height, button_dc, 0, 0, SRCCOPY);

        todo_wine_if(type == BS_PUSHBOX)
        ok(equal_dc(mem_dc1, mem_dc2, width, height), "Type %#lx: Expected content unchanged.\n", type);

        DeleteObject(mem_bitmap2);
        DeleteObject(mem_bitmap1);
        DeleteDC(mem_dc2);
        DeleteDC(mem_dc1);
        ReleaseDC(button, button_dc);
        DestroyWindow(button);
    }

    DestroyWindow(parent);
}

START_TEST(button)
{
    BOOL (WINAPI * pIsThemeActive)(VOID);
    ULONG_PTR ctx_cookie;
    HMODULE uxtheme;
    HANDLE hCtx;

    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;

    uxtheme = LoadLibraryA("uxtheme.dll");
    if (uxtheme)
    {
        pIsThemeActive = (void*)GetProcAddress(uxtheme, "IsThemeActive");
        if (pIsThemeActive)
            is_theme_active = pIsThemeActive();
        FreeLibrary(uxtheme);
    }

    register_parent_class();

    init_functions();
    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    test_button_class();
    test_button_messages();
    test_note();
    test_button_data();
    test_bm_get_set_image();
    test_get_set_imagelist();
    test_get_set_textmargin();
    test_state();
    test_bcm_get_ideal_size();
    test_style();
    test_visual();

    unload_v6_module(ctx_cookie, hCtx);
}
