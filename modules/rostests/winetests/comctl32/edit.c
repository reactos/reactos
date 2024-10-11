/* Unit test suite for edit control.
 *
 * Copyright 2004 Vitaliy Margolen
 * Copyright 2005 C. Scott Ananian
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
#include <imm.h>

#include "wine/test.h"
#include "v6util.h"
#include "msg.h"

#ifndef ES_COMBO
#define ES_COMBO 0x200
#endif

#define ID_EDITTESTDBUTTON 0x123
#define ID_EDITTEST2 99
#define MAXLEN 200

enum seq_index
{
    COMBINED_SEQ_INDEX = 0,
    NUM_MSG_SEQUENCES,
};

enum msg_id
{
    PARENT_ID,
    EDIT_ID,
};

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

struct edit_notify {
    int en_change, en_maxtext, en_update;
};

static struct edit_notify notifications;

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, min_timeout, QS_ALLINPUT) == WAIT_TIMEOUT)
            break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
}

static BOOL open_clipboard(HWND hwnd)
{
    DWORD start = GetTickCount();
    while (1)
    {
        BOOL ret = OpenClipboard(hwnd);
        if (ret || GetLastError() != ERROR_ACCESS_DENIED)
            return ret;
        if (GetTickCount() - start > 100)
        {
            char classname[256];
            DWORD le = GetLastError();
            HWND clipwnd = GetOpenClipboardWindow();
            /* Provide a hint as to the source of interference:
             * - The class name would typically be CLIPBRDWNDCLASS if the
             *   clipboard was opened by a Windows application using the
             *   ole32 API.
             * - And it would be __wine_clipboard_manager if it was opened in
             *   response to a native application.
             */
            GetClassNameA(clipwnd, classname, ARRAY_SIZE(classname));
            trace("%p (%s) opened the clipboard\n", clipwnd, classname);
            SetLastError(le);
            return ret;
        }
        Sleep(15);
    }
}

static INT_PTR CALLBACK multi_edit_dialog_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static int num_ok_commands = 0;
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND hedit = GetDlgItem(hdlg, 1000);
            SetFocus(hedit);
            switch (lparam)
            {
                /* test cases related to bug 12319 */
                case 0:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 1:
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 2:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;

                /* test cases for pressing enter */
                case 3:
                    num_ok_commands = 0;
                    PostMessageA(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                default:
                    break;
            }
            break;
        }

        case WM_COMMAND:
            if (HIWORD(wparam) != BN_CLICKED)
                break;

            switch (LOWORD(wparam))
            {
                case IDOK:
                    num_ok_commands++;
                    break;

                default:
                    break;
            }
            break;

        case WM_USER:
        {
            HWND hfocus = GetFocus();
            HWND hedit = GetDlgItem(hdlg, 1000);
            HWND hedit2 = GetDlgItem(hdlg, 1001);
            HWND hedit3 = GetDlgItem(hdlg, 1002);

            if (wparam != 0xdeadbeef)
                break;

            switch (lparam)
            {
                case 0:
                    if (hfocus == hedit)
                        EndDialog(hdlg, 1111);
                    else if (hfocus == hedit2)
                        EndDialog(hdlg, 2222);
                    else if (hfocus == hedit3)
                        EndDialog(hdlg, 3333);
                    else
                        EndDialog(hdlg, 4444);
                    break;
                case 1:
                    if ((hfocus == hedit) && (num_ok_commands == 0))
                        EndDialog(hdlg, 11);
                    else
                        EndDialog(hdlg, 22);
                    break;
                default:
                    EndDialog(hdlg, 5555);
            }
            break;
        }

        case WM_CLOSE:
            EndDialog(hdlg, 333);
            break;

        default:
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK edit_dialog_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND hedit = GetDlgItem(hdlg, 1000);
            SetFocus(hedit);
            switch (lparam)
            {
                /* from bug 11841 */
                case 0:
                    PostMessageA(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    break;
                case 1:
                    PostMessageA(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    break;
                case 2:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                /* more test cases for WM_CHAR */
                case 3:
                    PostMessageA(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 4:
                    PostMessageA(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 5:
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;

                /* more test cases for WM_KEYDOWN + WM_CHAR */
                case 6:
                    PostMessageA(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    PostMessageA(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 7:
                    PostMessageA(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessageA(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;
                case 8:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                /* multiple tab tests */
                case 9:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 2);
                    break;
                case 10:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 2);
                    break;

                default:
                    break;
            }
            break;
        }

        case WM_COMMAND:
            if (HIWORD(wparam) != BN_CLICKED)
                break;

            switch (LOWORD(wparam))
            {
                case IDOK:
                    EndDialog(hdlg, 111);
                    break;

                case IDCANCEL:
                    EndDialog(hdlg, 222);
                    break;

                default:
                    break;
            }
            break;

        case WM_USER:
        {
            int len;
            HWND hok = GetDlgItem(hdlg, IDOK);
            HWND hcancel = GetDlgItem(hdlg, IDCANCEL);
            HWND hedit = GetDlgItem(hdlg, 1000);
            HWND hfocus = GetFocus();

            if (wparam != 0xdeadbeef)
                break;

            switch (lparam)
            {
                case 0:
                    len = SendMessageA(hedit, WM_GETTEXTLENGTH, 0, 0);
                    if (len == 0)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                case 1:
                    len = SendMessageA(hedit, WM_GETTEXTLENGTH, 0, 0);
                    if ((hfocus == hok) && len == 0)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                case 2:
                    if (hfocus == hok)
                        EndDialog(hdlg, 11);
                    else if (hfocus == hcancel)
                        EndDialog(hdlg, 22);
                    else if (hfocus == hedit)
                        EndDialog(hdlg, 33);
                    else
                        EndDialog(hdlg, 44);
                    break;

                default:
                    EndDialog(hdlg, 555);
            }
            break;
        }

        case WM_CLOSE:
            EndDialog(hdlg, 333);
            break;

        default:
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK edit_singleline_dialog_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND hedit = GetDlgItem(hdlg, 1000);
            SetFocus(hedit);
            switch (lparam)
            {
                /* test cases for WM_KEYDOWN */
                case 0:
                    PostMessageA(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    break;
                case 1:
                    PostMessageA(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    break;
                case 2:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                /* test cases for WM_CHAR */
                case 3:
                    PostMessageA(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 4:
                    PostMessageA(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 5:
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;

                /* test cases for WM_KEYDOWN + WM_CHAR */
                case 6:
                    PostMessageA(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    PostMessageA(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    break;
                case 7:
                    PostMessageA(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessageA(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    break;
                case 8:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                default:
                    break;
            }
            break;
        }

        case WM_COMMAND:
            if (HIWORD(wparam) != BN_CLICKED)
                break;

            switch (LOWORD(wparam))
            {
                case IDOK:
                    EndDialog(hdlg, 111);
                    break;

                case IDCANCEL:
                    EndDialog(hdlg, 222);
                    break;

                default:
                    break;
            }
            break;

        case WM_USER:
        {
            HWND hok = GetDlgItem(hdlg, IDOK);
            HWND hedit = GetDlgItem(hdlg, 1000);
            HWND hfocus = GetFocus();
            int len = SendMessageA(hedit, WM_GETTEXTLENGTH, 0, 0);

            if (wparam != 0xdeadbeef)
                break;

            switch (lparam)
            {
                case 0:
                    if ((hfocus == hedit) && len == 0)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                case 1:
                    if ((hfocus == hok) && len == 0)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                default:
                    EndDialog(hdlg, 55);
            }
            break;
        }

        case WM_CLOSE:
            EndDialog(hdlg, 333);
            break;

        default:
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK edit_wantreturn_dialog_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND hedit = GetDlgItem(hdlg, 1000);
            SetFocus(hedit);
            switch (lparam)
            {
                /* test cases for WM_KEYDOWN */
                case 0:
                    PostMessageA(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    break;
                case 1:
                    PostMessageA(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 2:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                /* test cases for WM_CHAR */
                case 3:
                    PostMessageA(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 4:
                    PostMessageA(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 2);
                    break;
                case 5:
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;

                /* test cases for WM_KEYDOWN + WM_CHAR */
                case 6:
                    PostMessageA(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    PostMessageA(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 7:
                    PostMessageA(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessageA(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 2);
                    break;
                case 8:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                default:
                    break;
            }
            break;
        }

        case WM_COMMAND:
            if (HIWORD(wparam) != BN_CLICKED)
                break;

            switch (LOWORD(wparam))
            {
                case IDOK:
                    EndDialog(hdlg, 111);
                    break;

                case IDCANCEL:
                    EndDialog(hdlg, 222);
                    break;

                default:
                    break;
            }
            break;

        case WM_USER:
        {
            HWND hok = GetDlgItem(hdlg, IDOK);
            HWND hedit = GetDlgItem(hdlg, 1000);
            HWND hfocus = GetFocus();
            int len = SendMessageA(hedit, WM_GETTEXTLENGTH, 0, 0);

            if (wparam != 0xdeadbeef)
                break;

            switch (lparam)
            {
                case 0:
                    if ((hfocus == hedit) && len == 0)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                case 1:
                    if ((hfocus == hok) && len == 0)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                case 2:
                    if ((hfocus == hedit) && len == 2)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                default:
                    EndDialog(hdlg, 55);
            }
            break;
        }

        case WM_CLOSE:
            EndDialog(hdlg, 333);
            break;

        default:
            break;
    }

    return FALSE;
}

static HINSTANCE hinst;
static HWND hwndET2;
static const char szEditTest2Class[] = "EditTest2Class";
static const char szEditTest3Class[] = "EditTest3Class";
static const char szEditTest4Class[] = "EditTest4Class";
static const char szEditTextPositionClass[] = "EditTextPositionWindowClass";

static HWND create_editcontrol (DWORD style, DWORD exstyle)
{
    HWND handle;

    handle = CreateWindowExA(exstyle, WC_EDITA, "Text Text", style, 10, 10, 300, 300,
        NULL, NULL, hinst, NULL);
    ok (handle != NULL, "CreateWindow EDIT Control failed\n");

    if (winetest_interactive)
	ShowWindow (handle, SW_SHOW);
    return handle;
}

static HWND create_editcontrolW(DWORD style, DWORD exstyle)
{
    static const WCHAR testtextW[] = {'T','e','s','t',' ','t','e','x','t',0};
    HWND handle;

    handle = CreateWindowExW(exstyle, WC_EDITW, testtextW, style, 10, 10, 300, 300,
        NULL, NULL, hinst, NULL);
    ok(handle != NULL, "Failed to create Edit control.\n");
    return handle;
}

static HWND create_child_editcontrol (DWORD style, DWORD exstyle)
{
    HWND parentWnd;
    HWND editWnd;
    RECT rect;
    BOOL b;
    SetRect(&rect, 0, 0, 300, 300);
    b = AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    ok(b, "AdjustWindowRect failed\n");

    parentWnd = CreateWindowExA(0,
                            szEditTextPositionClass,
                            "Edit Test",
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            rect.right - rect.left, rect.bottom - rect.top,
                            NULL, NULL, hinst, NULL);
    ok (parentWnd != NULL, "CreateWindow EDIT Test failed\n");

    editWnd = CreateWindowExA(exstyle,
                            "EDIT",
                            "Test Text",
                            WS_CHILD | style,
                            0, 0, 300, 300,
                            parentWnd, NULL, hinst, NULL);
    ok (editWnd != NULL, "CreateWindow EDIT Test Text failed\n");
    if (winetest_interactive)
        ShowWindow (parentWnd, SW_SHOW);
    return editWnd;
}

static void destroy_child_editcontrol (HWND hwndEdit)
{
    if (GetParent(hwndEdit))
        DestroyWindow(GetParent(hwndEdit));
    else {
        trace("Edit control has no parent!\n");
        DestroyWindow(hwndEdit);
    }
}

static LONG get_edit_style (HWND hwnd)
{
    return GetWindowLongA( hwnd, GWL_STYLE ) & (
	ES_LEFT |
/* FIXME: not implemented
	ES_CENTER |
	ES_RIGHT |
	ES_OEMCONVERT |
*/
	ES_MULTILINE |
	ES_UPPERCASE |
	ES_LOWERCASE |
	ES_PASSWORD |
	ES_AUTOVSCROLL |
	ES_AUTOHSCROLL |
	ES_NOHIDESEL |
	ES_COMBO |
	ES_READONLY |
	ES_WANTRETURN |
	ES_NUMBER
	);
}

static void set_client_height(HWND Wnd, unsigned Height)
{
    RECT ClientRect, WindowRect;

    GetWindowRect(Wnd, &WindowRect);
    GetClientRect(Wnd, &ClientRect);
    SetWindowPos(Wnd, NULL, 0, 0,
                 WindowRect.right - WindowRect.left,
                 Height + (WindowRect.bottom - WindowRect.top) -
                 (ClientRect.bottom - ClientRect.top),
                 SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);

    /* Workaround for a bug in Windows' edit control
       (multi-line mode) */
    GetWindowRect(Wnd, &WindowRect);
    SetWindowPos(Wnd, NULL, 0, 0,
                 WindowRect.right - WindowRect.left + 1,
                 WindowRect.bottom - WindowRect.top + 1,
                 SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    SetWindowPos(Wnd, NULL, 0, 0,
                 WindowRect.right - WindowRect.left,
                 WindowRect.bottom - WindowRect.top,
                 SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);

    GetClientRect(Wnd, &ClientRect);
    ok(ClientRect.bottom - ClientRect.top == Height,
        "The client height should be %d, but is %ld\n",
        Height, ClientRect.bottom - ClientRect.top);
}

static void test_edit_control_1(void)
{
    HWND hwEdit;
    MSG msMessage;
    int i;
    LONG r;

    msMessage.message = WM_KEYDOWN;

    trace("EDIT: Single line\n");
    hwEdit = create_editcontrol(ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_AUTOVSCROLL | ES_AUTOHSCROLL), "Wrong style expected 0xc0 got: 0x%lx\n", r);
    for (i = 0; i < 65535; i++)
    {
        msMessage.wParam = i;
        r = SendMessageA(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
        ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS),
            "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS got %lx\n", r);
    }
    DestroyWindow(hwEdit);

    trace("EDIT: Single line want returns\n");
    hwEdit = create_editcontrol(ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN), "Wrong style expected 0x10c0 got: 0x%lx\n", r);
    for (i = 0; i < 65535; i++)
    {
        msMessage.wParam = i;
        r = SendMessageA(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
        ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS),
            "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS got %lx\n", r);
    }
    DestroyWindow(hwEdit);

    trace("EDIT: Multiline line\n");
    hwEdit = create_editcontrol(ES_MULTILINE | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE), "Wrong style expected 0xc4 got: 0x%lx\n", r);
    for (i = 0; i < 65535; i++)
    {
        msMessage.wParam = i;
        r = SendMessageA(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
        ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS),
            "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS got %lx\n", r);
    }
    DestroyWindow(hwEdit);

    trace("EDIT: Multi line want returns\n");
    hwEdit = create_editcontrol(ES_MULTILINE | WS_VSCROLL | ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE), "Wrong style expected 0x10c4 got: 0x%lx\n", r);
    for (i = 0; i < 65535; i++)
    {
        msMessage.wParam = i;
        r = SendMessageA(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
        ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS),
            "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS got %lx\n", r);
    }
    DestroyWindow(hwEdit);
}

/* WM_SETTEXT is implemented by selecting all text, and then replacing the
 * selection.  This test checks that the first 'select all' doesn't generate
 * an UPDATE message which can escape and (via a handler) change the
 * selection, which would cause WM_SETTEXT to break.  This old bug
 * was fixed 18-Mar-2005; we check here to ensure it doesn't regress.
 */
static void test_edit_control_2(void)
{
    HWND hwndMain, phwnd;
    char szLocalString[MAXLEN];
    LONG r, w = 150, h = 50;
    POINT cpos;

    /* Create main and edit windows. */
    hwndMain = CreateWindowA(szEditTest2Class, "ET2", WS_OVERLAPPEDWINDOW,
                            0, 0, 200, 200, NULL, NULL, hinst, NULL);
    ok(hwndMain != NULL, "Failed to create control parent.\n");
    if (winetest_interactive)
        ShowWindow (hwndMain, SW_SHOW);

    hwndET2 = CreateWindowA(WC_EDITA, NULL, WS_CHILD|WS_BORDER|ES_LEFT|ES_AUTOHSCROLL,
        0, 0, w, h, /* important this not be 0 size. */
        hwndMain, (HMENU) ID_EDITTEST2, hinst, NULL);
    ok(hwndET2 != NULL, "Failed to create Edit control.\n");
    if (winetest_interactive)
        ShowWindow (hwndET2, SW_SHOW);

    trace("EDIT: SETTEXT atomicity\n");
    /* Send messages to "type" in the word 'foo'. */
    r = SendMessageA(hwndET2, WM_CHAR, 'f', 1);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);
    r = SendMessageA(hwndET2, WM_CHAR, 'o', 1);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);
    r = SendMessageA(hwndET2, WM_CHAR, 'o', 1);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);
    /* 'foo' should have been changed to 'bar' by the UPDATE handler. */
    GetWindowTextA(hwndET2, szLocalString, MAXLEN);
    ok(strcmp(szLocalString, "bar")==0,
       "Wrong contents of edit: %s\n", szLocalString);

    /* try setting the caret before it's visible */
    r = SetCaretPos(0, 0);
    todo_wine ok(0 == r, "SetCaretPos succeeded unexpectedly, expected: 0, got: %ld\n", r);
    phwnd = SetFocus(hwndET2);
    ok(phwnd != NULL, "SetFocus failed unexpectedly, expected non-zero, got NULL\n");
    r = SetCaretPos(0, 0);
    ok(1 == r, "SetCaretPos failed unexpectedly, expected: 1, got: %ld\n", r);
    r = GetCaretPos(&cpos);
    ok(1 == r, "GetCaretPos failed unexpectedly, expected: 1, got: %ld\n", r);
    ok(cpos.x == 0 && cpos.y == 0, "Wrong caret position, expected: (0,0), got: (%ld,%ld)\n", cpos.x, cpos.y);
    r = SetCaretPos(-1, -1);
    ok(1 == r, "SetCaretPos failed unexpectedly, expected: 1, got: %ld\n", r);
    r = GetCaretPos(&cpos);
    ok(1 == r, "GetCaretPos failed unexpectedly, expected: 1, got: %ld\n", r);
    ok(cpos.x == -1 && cpos.y == -1, "Wrong caret position, expected: (-1,-1), got: (%ld,%ld)\n", cpos.x, cpos.y);
    r = SetCaretPos(w << 1, h << 1);
    ok(1 == r, "SetCaretPos failed unexpectedly, expected: 1, got: %ld\n", r);
    r = GetCaretPos(&cpos);
    ok(1 == r, "GetCaretPos failed unexpectedly, expected: 1, got: %ld\n", r);
    ok(cpos.x == (w << 1) && cpos.y == (h << 1), "Wrong caret position, expected: (%ld,%ld), got: (%ld,%ld)\n", w << 1, h << 1, cpos.x, cpos.y);
    r = SetCaretPos(w, h);
    ok(1 == r, "SetCaretPos failed unexpectedly, expected: 1, got: %ld\n", r);
    r = GetCaretPos(&cpos);
    ok(1 == r, "GetCaretPos failed unexpectedly, expected: 1, got: %ld\n", r);
    ok(cpos.x == w && cpos.y == h, "Wrong caret position, expected: (%ld,%ld), got: (%ld,%ld)\n", w, h, cpos.x, cpos.y);
    r = SetCaretPos(w - 1, h - 1);
    ok(1 == r, "SetCaretPos failed unexpectedly, expected: 1, got: %ld\n", r);
    r = GetCaretPos(&cpos);
    ok(1 == r, "GetCaretPos failed unexpectedly, expected: 1, got: %ld\n", r);
    ok(cpos.x == (w - 1) && cpos.y == (h - 1), "Wrong caret position, expected: (%ld,%ld), got: (%ld,%ld)\n", w - 1, h - 1, cpos.x, cpos.y);

    DestroyWindow(hwndET2);
    DestroyWindow(hwndMain);
}

static void ET2_check_change(void)
{
   char szLocalString[MAXLEN];
   /* This EN_UPDATE handler changes any 'foo' to 'bar'. */
   GetWindowTextA(hwndET2, szLocalString, MAXLEN);
   if (!strcmp(szLocalString, "foo"))
   {
       strcpy(szLocalString, "bar");
       SendMessageA(hwndET2, WM_SETTEXT, 0, (LPARAM)szLocalString);
   }
   /* always leave the cursor at the end. */
   SendMessageA(hwndET2, EM_SETSEL, MAXLEN - 1, MAXLEN - 1);
}

static void ET2_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (id == ID_EDITTEST2 && codeNotify == EN_UPDATE)
        ET2_check_change();
}

static LRESULT CALLBACK ET2_WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
    case WM_COMMAND:
        ET2_OnCommand(hwnd, LOWORD(wParam), (HWND)lParam, HIWORD(wParam));
        break;
    }
    return DefWindowProcA(hwnd, iMsg, wParam, lParam);
}

static void zero_notify(void)
{
    notifications.en_change = 0;
    notifications.en_maxtext = 0;
    notifications.en_update = 0;
}

#define test_notify(enchange, enmaxtext, enupdate) \
do { \
    ok(notifications.en_change == enchange, "expected %d EN_CHANGE notifications, " \
    "got %d\n", enchange, notifications.en_change); \
    ok(notifications.en_maxtext == enmaxtext, "expected %d EN_MAXTEXT notifications, " \
    "got %d\n", enmaxtext, notifications.en_maxtext); \
    ok(notifications.en_update == enupdate, "expected %d EN_UPDATE notifications, " \
    "got %d\n", enupdate, notifications.en_update); \
} while(0)

static LRESULT CALLBACK edit3_wnd_procA(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case EN_MAXTEXT:
                    notifications.en_maxtext++;
                    break;
                case EN_UPDATE:
                    notifications.en_update++;
                    break;
                case EN_CHANGE:
                    notifications.en_change++;
                    break;
            }
            break;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

static LRESULT CALLBACK parent_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    struct message msg = { 0 };
    LRESULT ret;

    msg.message = message;
    msg.flags = sent|wparam|id;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.id = PARENT_ID;

    if (message != WM_IME_SETCONTEXT &&
        message != WM_IME_NOTIFY &&
        message != WM_GETICON &&
        message != WM_DWMNCRENDERINGCHANGED &&
        message != WM_GETMINMAXINFO &&
        message != WM_PAINT &&
        message != WM_CTLCOLOREDIT &&
        message != WM_WINDOWPOSCHANGING &&
        message != WM_WINDOWPOSCHANGED &&
        message != WM_MOVE &&
        message != WM_MOUSEACTIVATE &&
        message < 0xc000)
    {
        add_message(sequences, COMBINED_SEQ_INDEX, &msg);
    }

    defwndproc_counter++;
    ret = DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static LRESULT CALLBACK edit_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    struct message msg = { 0 };
    LRESULT ret;

    msg.message = message;
    msg.flags = sent|wparam|id;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.id = EDIT_ID;

    if (message != WM_IME_SETCONTEXT &&
        message != WM_IME_NOTIFY)
    {
        add_message(sequences, COMBINED_SEQ_INDEX, &msg);
    }

    defwndproc_counter++;
    if (IsWindowUnicode(hwnd))
        ret = CallWindowProcW(oldproc, hwnd, message, wParam, lParam);
    else
        ret = CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

/* Test behaviour of WM_SETTEXT, WM_REPLACESEL and notifications sent in response
 * to these messages.
 */
static void test_edit_control_3(void)
{
    static const char *str = "this is a long string.";
    static const char *str2 = "this is a long string.\r\nthis is a long string.\r\nthis is a long string.\r\nthis is a long string.";
    HWND hWnd, hParent;
    int len, dpi;
    HDC hDC;

    hDC = GetDC(NULL);
    dpi = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(NULL, hDC);

    trace("EDIT: Test notifications\n");

    hParent = CreateWindowExA(0,
              szEditTest3Class,
              NULL,
              0,
              CW_USEDEFAULT, CW_USEDEFAULT, 10, 10,
              NULL, NULL, NULL, NULL);
    ok(hParent != NULL, "Failed to create control parent.\n");

    trace("EDIT: Single line, no ES_AUTOHSCROLL\n");
    hWnd = CreateWindowExA(0, WC_EDITA, NULL, 0, 10, 10, 50, 50, hParent, NULL, NULL, NULL);
    ok(hWnd != NULL, "Failed to create Edit control.\n");

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    if (len == lstrlenA(str)) /* Win 8 */
        test_notify(1, 0, 1);
    else
        test_notify(1, 1, 1);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)"a");
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(1 == len, "wrong text length, expected 1, got %d\n", len);
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    len = SendMessageA(hWnd, EM_GETSEL, 0, 0);
    ok(LOWORD(len)==0, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==0, "Unexpected end position for selection %d\n", HIWORD(len));
    SendMessageA(hParent, WM_SETFOCUS, 0, (LPARAM)hWnd);
    len = SendMessageA(hWnd, EM_GETSEL, 0, 0);
    ok(LOWORD(len)==0, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==0, "Unexpected end position for selection %d\n", HIWORD(len));

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    DestroyWindow(hWnd);

    trace("EDIT: Single line, ES_AUTOHSCROLL\n");
    hWnd = CreateWindowExA(0, WC_EDITA, NULL, ES_AUTOHSCROLL, 10, 10, 50, 50, hParent, NULL, NULL, NULL);
    ok(hWnd != NULL, "Failed to create Edit control.\n");

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    DestroyWindow(hWnd);

    trace("EDIT: Multline, no ES_AUTOHSCROLL, no ES_AUTOVSCROLL\n");
    hWnd = CreateWindowExA(0, WC_EDITA, NULL, ES_MULTILINE,
              10, 10, (50 * dpi) / 96, (50 * dpi) / 96,
              hParent, NULL, NULL, NULL);
    ok(hWnd != NULL, "Failed to create Edit control.\n");

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    if (len == lstrlenA(str)) /* Win 8 */
        test_notify(1, 0, 1);
    else
    {
        ok(0 == len, "text should have been truncated, expected 0, got %d\n", len);
        test_notify(1, 1, 1);
    }

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)"a");
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(1 == SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0), "wrong text length, expected 1, got %d\n", len);
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    DestroyWindow(hWnd);

    trace("EDIT: Multline, ES_AUTOHSCROLL, no ES_AUTOVSCROLL\n");
    hWnd = CreateWindowExA(0, WC_EDITA, NULL, ES_MULTILINE | ES_AUTOHSCROLL,
              10, 10, (50 * dpi) / 96, (50 * dpi) / 96,
              hParent, NULL, NULL, NULL);
    ok(hWnd != NULL, "Failed to create Edit control.\n");

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(0 == len, "text should have been truncated, expected 0, got %d\n", len);
    test_notify(1, 1, 1);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)"a");
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(1 == SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0), "wrong text length, expected 1, got %d\n", len);
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    DestroyWindow(hWnd);

    trace("EDIT: Multline, ES_AUTOHSCROLL and ES_AUTOVSCROLL\n");
    hWnd = CreateWindowExA(0, WC_EDITA, NULL, ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
              10, 10, 50, 50, hParent, NULL, NULL, NULL);
    ok(hWnd != NULL, "Failed to create Edit control.\n");

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    DestroyWindow(hWnd);
}

static void test_char_from_pos(void)
{
    int lo, hi, mid, ret, i;
    HWND hwEdit;
    HDC dc;
    SIZE size;

    hwEdit = create_editcontrol(ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessageA(hwEdit, WM_SETTEXT, 0, (LPARAM)"aa");
    lo = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo + 1) / 2;

    for (i = lo; i < mid; i++)
    {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }

    for (i = mid; i <= hi; i++)
    {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }

    ret = SendMessageA(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_RIGHT | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessageA(hwEdit, WM_SETTEXT, 0, (LPARAM)"aa");
    lo = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo + 1) / 2;

    for (i = lo; i < mid; i++)
    {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }

    for (i = mid; i <= hi; i++)
    {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }

    ret = SendMessageA(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_CENTER | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessageA(hwEdit, WM_SETTEXT, 0, (LPARAM)"aa");
    lo = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo + 1) / 2;

    for (i = lo; i < mid; i++)
    {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }

    for (i = mid; i <= hi; i++)
    {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }

    ret = SendMessageA(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessageA(hwEdit, WM_SETTEXT, 0, (LPARAM)"aa");
    lo = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo + 1) / 2;

    for (i = lo; i < mid; i++)
    {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok((0 == ret || 1 == ret /* Vista */), "expected 0 or 1 got %d\n", ret);
    }

    for (i = mid; i <= hi; i++)
    {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }

    ret = SendMessageA(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_MULTILINE | ES_RIGHT | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessageA(hwEdit, WM_SETTEXT, 0, (LPARAM)"aa");
    lo = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo + 1) / 2;

    for (i = lo; i < mid; i++)
    {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok((0 == ret || 1 == ret /* Vista */), "expected 0 or 1 got %d\n", ret);
    }

    for (i = mid; i <= hi; i++)
    {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }

    ret = SendMessageA(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_MULTILINE | ES_CENTER | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessageA(hwEdit, WM_SETTEXT, 0, (LPARAM)"aa");
    lo = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo + 1) / 2;

    for (i = lo; i < mid; i++)
    {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok((0 == ret || 1 == ret /* Vista */), "expected 0 or 1 got %d\n", ret);
    }

    for (i = mid; i <= hi; i++)
    {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }

    ret = SendMessageA(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    /* Scrolled to the right with partially visible line, position on next line. */
    hwEdit = create_editcontrol(ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);

    dc = GetDC(hwEdit);
    GetTextExtentPoint32A(dc, "w", 1, &size);
    ReleaseDC(hwEdit, dc);

    SetWindowPos(hwEdit, NULL, 0, 0, size.cx * 15, size.cy * 5, SWP_NOMOVE | SWP_NOZORDER);
    SendMessageA(hwEdit, WM_SETTEXT, 0, (LPARAM)"wwwwwwwwwwwwwwwwwwww\r\n\r\n");
    SendMessageA(hwEdit, EM_SETSEL, 40, 40);

    lo = (short)SendMessageA(hwEdit, EM_POSFROMCHAR, 22, 0);
    ret = (short)SendMessageA(hwEdit, EM_POSFROMCHAR, 20, 0);
    ret -= 20 * size.cx; /* Calculate expected position, 20 characters back. */
    ok(ret == lo, "Unexpected position %d vs %d.\n", lo, ret);

    DestroyWindow(hwEdit);
}

/* Test if creating edit control without ES_AUTOHSCROLL and ES_AUTOVSCROLL
 * truncates text that doesn't fit.
 */
static void test_edit_control_5(void)
{
    static const char *str = "test\r\ntest";
    HWND parentWnd;
    HWND hWnd;
    int len;
    RECT rc1 = { 10, 10, 11, 11};
    RECT rc;

    /* first show that a non-child won't do for this test */
    hWnd = CreateWindowExA(0, WC_EDITA, str, 0, 10, 10, 1, 1, NULL, NULL, NULL, NULL);
    ok(hWnd != NULL, "Failed to create Edit control.\n");

    /* size of non-child edit control is (much) bigger than requested */
    GetWindowRect( hWnd, &rc);
    ok( rc.right - rc.left > 20, "size of the window (%ld) is smaller than expected\n",
            rc.right - rc.left);
    DestroyWindow(hWnd);
    /* so create a parent, and give it edit controls children to test with */
    parentWnd = CreateWindowExA(0,
                            szEditTextPositionClass,
                            "Edit Test", WS_VISIBLE |
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            250, 250,
                            NULL, NULL, hinst, NULL);
    ok(parentWnd != NULL, "Failed to create control parent.\n");
    ShowWindow( parentWnd, SW_SHOW);
    /* single line */
    hWnd = CreateWindowExA(0, WC_EDITA, str, WS_VISIBLE | WS_BORDER | WS_CHILD,
              rc1.left, rc1.top, rc1.right - rc1.left, rc1.bottom - rc1.top,
              parentWnd, NULL, NULL, NULL);
    ok(hWnd != NULL, "Failed to create Edit control.\n");
    GetClientRect( hWnd, &rc);
    ok( rc.right == rc1.right - rc1.left && rc.bottom == rc1.bottom - rc1.top,
            "Client rectangle not the expected size %s\n", wine_dbgstr_rect( &rc ));
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    DestroyWindow(hWnd);
    /* multi line */
    hWnd = CreateWindowExA(0, WC_EDITA, str, WS_CHILD | ES_MULTILINE,
              rc1.left, rc1.top, rc1.right - rc1.left, rc1.bottom - rc1.top,
              parentWnd, NULL, NULL, NULL);
    ok(hWnd != NULL, "Failed to create Edit control.\n");
    GetClientRect( hWnd, &rc);
    ok( rc.right == rc1.right - rc1.left && rc.bottom == rc1.bottom - rc1.top,
            "Client rectangle not the expected size %s\n", wine_dbgstr_rect( &rc ));
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    DestroyWindow(hWnd);
    DestroyWindow(parentWnd);
}

/* Test WM_GETTEXT processing
 * after destroy messages
 */
static void test_edit_control_6(void)
{
    static const char *str = "test\r\ntest";
    char buf[MAXLEN];
    HWND hWnd;
    LONG ret;

    hWnd = CreateWindowExA(0, "EDIT", "Test", 0, 10, 10, 1, 1, NULL, NULL, hinst, NULL);
    ok(hWnd != NULL, "Failed to create edit control.\n");

    ret = SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    ok(ret == TRUE, "Expected %d, got %ld\n", TRUE, ret);
    ret = SendMessageA(hWnd, WM_GETTEXT, MAXLEN, (LPARAM)buf);
    ok(ret == strlen(str), "Expected %s, got len %ld\n", str, ret);
    ok(!strcmp(buf, str), "Expected %s, got %s\n", str, buf);

    buf[0] = 0;
    ret = SendMessageA(hWnd, WM_DESTROY, 0, 0);
    todo_wine
    ok(ret == 1, "Unexpected return value %ld\n", ret);
    ret = SendMessageA(hWnd, WM_GETTEXT, MAXLEN, (LPARAM)buf);
    ok(ret == strlen(str), "Expected %s, got len %ld\n", str, ret);
    ok(!strcmp(buf, str), "Expected %s, got %s\n", str, buf);

    buf[0] = 0;
    ret = SendMessageA(hWnd, WM_NCDESTROY, 0, 0);
    ok(ret == 0, "Expected 0, got %ld\n", ret);
    ret = SendMessageA(hWnd, WM_GETTEXT, MAXLEN, (LPARAM)buf);
todo_wine {
    ok(ret == strlen("Test"), "Unexpected text length %ld\n", ret);
    ok(!strcmp(buf, "Test"), "Unexpected text %s\n", buf);
}
    DestroyWindow(hWnd);
}

static void test_edit_control_limittext(void)
{
    HWND hwEdit;
    DWORD r;

    /* Test default limit for single-line control */
    trace("EDIT: buffer limit for single-line\n");
    hwEdit = create_editcontrol(ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = SendMessageA(hwEdit, EM_GETLIMITTEXT, 0, 0);
    ok(r == 30000, "Incorrect default text limit, expected 30000 got %lu\n", r);
    SendMessageA(hwEdit, EM_SETLIMITTEXT, 0, 0);
    r = SendMessageA(hwEdit, EM_GETLIMITTEXT, 0, 0);
    ok( r == 2147483646, "got limit %lu (expected 2147483646)\n", r);
    DestroyWindow(hwEdit);

    /* Test default limit for multi-line control */
    trace("EDIT: buffer limit for multi-line\n");
    hwEdit = create_editcontrol(ES_MULTILINE | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = SendMessageA(hwEdit, EM_GETLIMITTEXT, 0, 0);
    ok(r == 30000, "Incorrect default text limit, expected 30000 got %lu\n", r);
    SendMessageA(hwEdit, EM_SETLIMITTEXT, 0, 0);
    r = SendMessageA(hwEdit, EM_GETLIMITTEXT, 0, 0);
    ok( r == 4294967295U, "got limit %lu (expected 4294967295)\n", r);
    DestroyWindow(hwEdit);
}

/* Test EM_SCROLL */
static void test_edit_control_scroll(void)
{
    static const char *single_line_str = "a";
    static const char *multiline_str = "Test\r\nText";
    HWND hwEdit;
    LONG ret;

    /* Check the return value when EM_SCROLL doesn't scroll
     * anything. Should not return true unless any lines were actually
     * scrolled. */
    hwEdit = CreateWindowA(WC_EDITA, single_line_str, WS_VSCROLL | ES_MULTILINE,
              1, 1, 100, 100, NULL, NULL, hinst, NULL);
    ok(hwEdit != NULL, "Failed to create Edit control.\n");

    ret = SendMessageA(hwEdit, EM_SCROLL, SB_PAGEDOWN, 0);
    ok(!ret, "Returned %lx, expected 0.\n", ret);

    ret = SendMessageA(hwEdit, EM_SCROLL, SB_PAGEUP, 0);
    ok(!ret, "Returned %lx, expected 0.\n", ret);

    ret = SendMessageA(hwEdit, EM_SCROLL, SB_LINEUP, 0);
    ok(!ret, "Returned %lx, expected 0.\n", ret);

    ret = SendMessageA(hwEdit, EM_SCROLL, SB_LINEDOWN, 0);
    ok(!ret, "Returned %lx, expected 0.\n", ret);

    DestroyWindow (hwEdit);

    /* SB_PAGEDOWN while at the beginning of a buffer with few lines
       should not cause EM_SCROLL to return a negative value of
       scrolled lines that would put us "before" the beginning. */
    hwEdit = CreateWindowA(WC_EDITA, multiline_str, WS_VSCROLL | ES_MULTILINE,
                0, 0, 100, 100, NULL, NULL, hinst, NULL);
    ok(hwEdit != NULL, "Failed to create Edit control.\n");

    ret = SendMessageA(hwEdit, EM_SCROLL, SB_PAGEDOWN, 0);
    ok(!ret, "Returned %lx, expected 0.\n", ret);

    DestroyWindow (hwEdit);
}

static BOOL is_cjk(HDC dc)
{
    const DWORD FS_DBCS_MASK = FS_JISJAPAN|FS_CHINESESIMP|FS_WANSUNG|FS_CHINESETRAD|FS_JOHAB;
    FONTSIGNATURE fs;

    switch (GdiGetCodePage(dc)) {
    case 932: case 936: case 949: case 950: case 1361:
        return TRUE;
    default:
        return (GetTextCharsetInfo(dc, &fs, 0) != DEFAULT_CHARSET &&
                (fs.fsCsb[0] & FS_DBCS_MASK));
    }
}

static void test_margins_usefontinfo(UINT charset)
{
    HWND hwnd;
    HDC hdc;
    TEXTMETRICW tm;
    SIZE size;
    LOGFONTA lf;
    HFONT hfont;
    RECT rect;
    INT margins, threshold, expect, empty_expect;
    const UINT small_margins = MAKELONG(1, 5);

    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = -11;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = charset;
    strcpy(lf.lfFaceName, "Tahoma");

    hfont = CreateFontIndirectA(&lf);
    ok(hfont != NULL, "got %p\n", hfont);

    /* Big window rectangle */
    hwnd = CreateWindowExA(0, "Edit", "A", WS_POPUP, 0, 0, 5000, 1000, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "got %p\n", hwnd);
    GetClientRect(hwnd, &rect);
    ok(!IsRectEmpty(&rect), "got rect %s\n", wine_dbgstr_rect(&rect));

    hdc = GetDC(hwnd);
    hfont = SelectObject(hdc, hfont);
    size.cx = GdiGetCharDimensions( hdc, &tm, &size.cy );
    if ((charset != tm.tmCharSet && charset != DEFAULT_CHARSET) ||
        !(tm.tmPitchAndFamily & (TMPF_TRUETYPE | TMPF_VECTOR))) {
        skip("%s for charset %d isn't available\n", lf.lfFaceName, charset);
        hfont = SelectObject(hdc, hfont);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        DeleteObject(hfont);
        return;
    }
    expect = MAKELONG(size.cx / 2, size.cx / 2);
    hfont = SelectObject(hdc, hfont);
    ReleaseDC(hwnd, hdc);

    margins = SendMessageA(hwnd, EM_GETMARGINS, 0, 0);
    ok(margins == 0, "got %x\n", margins);
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    SendMessageA(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(EC_USEFONTINFO, EC_USEFONTINFO));
    expect = SendMessageA(hwnd, EM_GETMARGINS, 0, 0);
    DestroyWindow(hwnd);

    threshold = HIWORD(expect) + LOWORD(expect) + size.cx * 2;
    empty_expect = threshold > 80 ? small_margins : expect;

    /* Size below the threshold, margins remain unchanged */
    hwnd = CreateWindowExA(0, "Edit", "A", WS_POPUP, 0, 0, threshold - 1, 100, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "got %p\n", hwnd);
    GetClientRect(hwnd, &rect);
    ok(!IsRectEmpty(&rect), "got rect %s\n", wine_dbgstr_rect(&rect));

    margins = SendMessageA(hwnd, EM_GETMARGINS, 0, 0);
    ok(margins == 0, "got %x\n", margins);

    SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    SendMessageA(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, small_margins);
    SendMessageA(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(EC_USEFONTINFO, EC_USEFONTINFO));
    margins = SendMessageA(hwnd, EM_GETMARGINS, 0, 0);
    ok(margins == small_margins, "%d: got %d, %d\n", charset, HIWORD(margins), LOWORD(margins));
    DestroyWindow(hwnd);

    /* Size at the threshold, margins become non-zero */
    hwnd = CreateWindowExA(0, "Edit", "A", WS_POPUP, 0, 0, threshold, 100, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "got %p\n", hwnd);
    GetClientRect(hwnd, &rect);
    ok(!IsRectEmpty(&rect), "got rect %s\n", wine_dbgstr_rect(&rect));

    margins = SendMessageA(hwnd, EM_GETMARGINS, 0, 0);
    ok(margins == 0, "got %x\n", margins);

    SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    SendMessageA(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, small_margins);
    SendMessageA(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(EC_USEFONTINFO, EC_USEFONTINFO));
    margins = SendMessageA(hwnd, EM_GETMARGINS, 0, 0);
    ok(margins == expect, "%d: got %d, %d\n", charset, HIWORD(margins), LOWORD(margins));
    DestroyWindow(hwnd);

    /* Empty rect */
    hwnd = CreateWindowExA(0, "Edit", "A", WS_POPUP, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "got %p\n", hwnd);
    GetClientRect(hwnd, &rect);
    ok(IsRectEmpty(&rect), "got rect %s\n", wine_dbgstr_rect(&rect));

    margins = SendMessageA(hwnd, EM_GETMARGINS, 0, 0);
    ok(margins == 0, "got %x\n", margins);

    SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    SendMessageA(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, small_margins);
    SendMessageA(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(EC_USEFONTINFO, EC_USEFONTINFO));
    margins = SendMessageA(hwnd, EM_GETMARGINS, 0, 0);
    ok(margins == empty_expect, "%d: got %d, %d\n", charset, HIWORD(margins), LOWORD(margins));
    DestroyWindow(hwnd);

    DeleteObject(hfont);
}

static INT get_cjk_fontinfo_margin(INT width, INT side_bearing)
{
    INT margin;
    if (side_bearing < 0)
        margin = min(-side_bearing, width/2);
    else
        margin = 0;
    return margin;
}

static DWORD get_cjk_font_margins(HDC hdc)
{
    ABC abc[256];
    SHORT left, right;
    UINT i;

    if (!GetCharABCWidthsW(hdc, 0, 255, abc))
        return 0;

    left = right = 0;
    for (i = 0; i < ARRAY_SIZE(abc); i++) {
        if (-abc[i].abcA > right) right = -abc[i].abcA;
        if (-abc[i].abcC > left)  left  = -abc[i].abcC;
    }
    return MAKELONG(left, right);
}

static void test_margins_default(const char* facename, UINT charset)
{
    HWND hwnd;
    HDC hdc;
    TEXTMETRICW tm;
    SIZE size;
    BOOL cjk;
    LOGFONTA lf;
    HFONT hfont;
    RECT rect;
    INT margins, expect, font_expect;
    const UINT small_margins = MAKELONG(1, 5);
    const WCHAR EditW[] = {'E','d','i','t',0}, strW[] = {'W',0};
    struct char_width_info {
        INT lsb, rsb, unknown;
    } info;
    HMODULE hgdi32;
    BOOL (WINAPI *pGetCharWidthInfo)(HDC, struct char_width_info *);

    hgdi32 = GetModuleHandleA("gdi32.dll");
    pGetCharWidthInfo = (void *)GetProcAddress(hgdi32, "GetCharWidthInfo");

    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = -11;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = charset;
    strcpy(lf.lfFaceName, facename);

    hfont = CreateFontIndirectA(&lf);
    ok(hfont != NULL, "got %p\n", hfont);

    /* Unicode version */
    hwnd = CreateWindowExW(0, EditW, strW, WS_POPUP, 0, 0, 5000, 1000, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "got %p\n", hwnd);
    GetClientRect(hwnd, &rect);
    ok(!IsRectEmpty(&rect), "got rect %s\n", wine_dbgstr_rect(&rect));

    hdc = GetDC(hwnd);
    hfont = SelectObject(hdc, hfont);
    size.cx = GdiGetCharDimensions( hdc, &tm, &size.cy );
    if ((charset != tm.tmCharSet && charset != DEFAULT_CHARSET) ||
        !(tm.tmPitchAndFamily & (TMPF_TRUETYPE | TMPF_VECTOR))) {
        skip("%s for charset %d isn't available\n", lf.lfFaceName, charset);
        hfont = SelectObject(hdc, hfont);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        DeleteObject(hfont);
        return;
    }
    cjk = is_cjk(hdc);
    if (cjk && pGetCharWidthInfo && pGetCharWidthInfo(hdc, &info)) {
        short left, right;

        left  = get_cjk_fontinfo_margin(size.cx, info.lsb);
        right = get_cjk_fontinfo_margin(size.cx, info.rsb);
        expect = MAKELONG(left, right);

        font_expect = get_cjk_font_margins(hdc);
        if (!font_expect)
            /* In this case, margins aren't updated */
            font_expect = small_margins;
    }
    else
        font_expect = expect = MAKELONG(size.cx / 2, size.cx / 2);

    hfont = SelectObject(hdc, hfont);
    ReleaseDC(hwnd, hdc);

    margins = SendMessageA(hwnd, EM_GETMARGINS, 0, 0);
    ok(margins == 0, "got %x\n", margins);
    SendMessageA(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, small_margins);
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    margins = SendMessageA(hwnd, EM_GETMARGINS, 0, 0);
    ok(margins == font_expect, "%s:%d: got %d, %d\n", facename, charset, HIWORD(margins), LOWORD(margins));
    SendMessageA(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, small_margins);
    SendMessageA(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(EC_USEFONTINFO, EC_USEFONTINFO));
    margins = SendMessageA(hwnd, EM_GETMARGINS, 0, 0);
    ok(margins == expect, "%s:%d: expected %d, %d, got %d, %d\n", facename, charset, HIWORD(expect), LOWORD(expect), HIWORD(margins), LOWORD(margins));
    DestroyWindow(hwnd);

    /* ANSI version */
    hwnd = CreateWindowExA(0, "Edit", "A", WS_POPUP, 0, 0, 5000, 1000, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "got %p\n", hwnd);
    GetClientRect(hwnd, &rect);
    ok(!IsRectEmpty(&rect), "got rect %s\n", wine_dbgstr_rect(&rect));

    margins = SendMessageA(hwnd, EM_GETMARGINS, 0, 0);
    ok(margins == 0, "got %x\n", margins);
    SendMessageA(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, small_margins);
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
    margins = SendMessageA(hwnd, EM_GETMARGINS, 0, 0);
    ok(margins == font_expect, "%s:%d: got %d, %d\n", facename, charset, HIWORD(margins), LOWORD(margins));
    SendMessageA(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, small_margins);
    SendMessageA(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(EC_USEFONTINFO, EC_USEFONTINFO));
    margins = SendMessageA(hwnd, EM_GETMARGINS, 0, 0);
    ok(margins == expect, "%s:%d: expected %d, %d, got %d, %d\n", facename, charset, HIWORD(expect), LOWORD(expect), HIWORD(margins), LOWORD(margins));
    DestroyWindow(hwnd);

    DeleteObject(hfont);
}

static INT CALLBACK find_font_proc(const LOGFONTA *elf, const TEXTMETRICA *ntm, DWORD type, LPARAM lParam)
{
    return 0;
}

static BOOL is_font_installed(const char*name)
{
    HDC hdc = GetDC(NULL);
    BOOL ret = FALSE;

    if (!EnumFontFamiliesA(hdc, name, find_font_proc, 0))
        ret = TRUE;

    ReleaseDC(NULL, hdc);
    return ret;
}

static WNDPROC orig_class_proc;

static LRESULT CALLBACK test_class_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    LRESULT result, r;

    switch (message)
    {
        case WM_NCCREATE:
            result = CallWindowProcA(orig_class_proc, hwnd, message, wParam, lParam);

            memset(&rect, 0, sizeof(rect));
            SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rect);
            ok(!rect.right && !rect.bottom, "Invalid size after NCCREATE: %ld x %ld\n", rect.right, rect.bottom);

            /* test that messages between WM_NCCREATE and WM_CREATE
               don't crash or cause unexpected behavior */
            r = SendMessageA(hwnd, EM_SETSEL, 0, 0);
            ok(r == 1, "Returned %Id, expected 1.\n", r);
            r = SendMessageA(hwnd, WM_SIZE, 0, 0x00100010);
            todo_wine ok(r == 1, "Returned %Id, expected 1.\n", r);
            r = SendMessageA(hwnd, EM_LINESCROLL, 1, 1);
            ok(r == 1, "Returned %Id, expected 1.\n", r);

            return result;

        case WM_CREATE:
            /* test that messages between WM_NCCREATE and WM_CREATE
               don't crash or cause unexpected behavior */
            r = SendMessageA(hwnd, EM_SETSEL, 0, 0);
            ok(r == 1, "Returned %Id, expected 1.\n", r);
            r = SendMessageA(hwnd, WM_SIZE, 0, 0x00100010);
            todo_wine ok(r == 1, "Returned %Id, expected 1.\n", r);
            r = SendMessageA(hwnd, EM_LINESCROLL, 1, 1);
            ok(r == 1, "Returned %Id, expected 1.\n", r);

            break;
    }

    return CallWindowProcA(orig_class_proc, hwnd, message, wParam, lParam);
}

static void test_initialization(void)
{
    BOOL ret;
    ATOM atom;
    HWND hwEdit;
    WNDCLASSA cls;

    ret = GetClassInfoA(NULL, "Edit", &cls);
    ok(ret, "Failed to get class info.\n");

    orig_class_proc = cls.lpfnWndProc;
    cls.lpfnWndProc = test_class_proc;
    cls.lpszClassName = "TestClassName";

    atom = RegisterClassA(&cls);
    ok(atom != 0, "Failed to register class.\n");

    hwEdit = CreateWindowExA(0, (LPCSTR)MAKEINTATOM(atom), "Text Text",
        ES_MULTILINE | WS_BORDER | ES_AUTOHSCROLL | ES_AUTOVSCROLL | WS_VSCROLL,
        10, 10, 300, 300, NULL, NULL, hinst, NULL);
    ok(hwEdit != NULL, "Failed to create a window.\n");

    DestroyWindow(hwEdit);
    UnregisterClassA((LPCSTR)MAKEINTATOM(atom), hinst);
}

static void test_margins(void)
{
    DWORD old_margins, new_margins;
    RECT old_rect, new_rect;
    INT old_right_margin;
    HWND hwEdit;

    hwEdit = create_editcontrol(WS_BORDER | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);

    old_margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    old_right_margin = HIWORD(old_margins);

    /* Check if setting the margins works */

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN, MAKELONG(10, 0));
    new_margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(new_margins) == 10, "Wrong left margin: %d\n", LOWORD(new_margins));
    ok(HIWORD(new_margins) == old_right_margin, "Wrong right margin: %d\n", HIWORD(new_margins));

    SendMessageA(hwEdit, EM_SETMARGINS, EC_RIGHTMARGIN, MAKELONG(0, 10));
    new_margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(new_margins) == 10, "Wrong left margin: %d\n", LOWORD(new_margins));
    ok(HIWORD(new_margins) == 10, "Wrong right margin: %d\n", HIWORD(new_margins));

    /* The size of the rectangle must decrease if we increase the margin */

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(5, 5));
    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM)&old_rect);
    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(15, 20));
    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM)&new_rect);
    ok(new_rect.left == old_rect.left + 10, "The left border of the rectangle is wrong\n");
    ok(new_rect.right == old_rect.right - 15, "The right border of the rectangle is wrong\n");
    ok(new_rect.top == old_rect.top, "The top border of the rectangle must not change\n");
    ok(new_rect.bottom == old_rect.bottom, "The bottom border of the rectangle must not change\n");

    /* If we set the margin to same value as the current margin,
       the rectangle must not change */

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(10, 10));
    SetRect(&old_rect, 1, 1, 99, 99);
    SendMessageA(hwEdit, EM_SETRECT, 0, (LPARAM)&old_rect);
    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM)&old_rect);
    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(10, 10));
    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM)&new_rect);
    ok(EqualRect(&old_rect, &new_rect), "The border of the rectangle has changed\n");

    /* The lParam argument of the WM_SIZE message should be ignored. */

    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM)&old_rect);
    SendMessageA(hwEdit, WM_SIZE, SIZE_RESTORED, 0);
    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM)&new_rect);
    ok(EqualRect(&old_rect, &new_rect), "The border of the rectangle has changed\n");
    SendMessageA(hwEdit, WM_SIZE, SIZE_MINIMIZED, 0);
    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM)&new_rect);
    ok(EqualRect(&old_rect, &new_rect), "The border of the rectangle has changed\n");
    SendMessageA(hwEdit, WM_SIZE, SIZE_MAXIMIZED, 0);
    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM)&new_rect);
    ok(EqualRect(&old_rect, &new_rect), "The border of the rectangle has changed\n");
    SendMessageA(hwEdit, WM_SIZE, SIZE_RESTORED, MAKELONG(10, 10));
    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM)&new_rect);
    ok(EqualRect(&old_rect, &new_rect), "The border of the rectangle has changed\n");

    DestroyWindow (hwEdit);

    test_margins_usefontinfo(ANSI_CHARSET);
    test_margins_usefontinfo(EASTEUROPE_CHARSET);

    test_margins_usefontinfo(SHIFTJIS_CHARSET);
    test_margins_usefontinfo(HANGUL_CHARSET);
    test_margins_usefontinfo(CHINESEBIG5_CHARSET);
    /* Don't test JOHAB_CHARSET.  Treated as CJK by Win 8,
       but not by < Win 8 and Win 10. */

    test_margins_usefontinfo(DEFAULT_CHARSET);

    test_margins_default("Tahoma", ANSI_CHARSET);
    test_margins_default("Tahoma", EASTEUROPE_CHARSET);

    test_margins_default("Tahoma", HANGUL_CHARSET);
    test_margins_default("Tahoma", CHINESEBIG5_CHARSET);

    if (is_font_installed("MS PGothic")) {
        test_margins_default("MS PGothic", SHIFTJIS_CHARSET);
        test_margins_default("MS PGothic", GREEK_CHARSET);
    }
    else
        skip("MS PGothic is not available, skipping some margin tests\n");

    if (is_font_installed("Ume P Gothic")) {
        test_margins_default("Ume P Gothic", SHIFTJIS_CHARSET);
        test_margins_default("Ume P Gothic", GREEK_CHARSET);
    }
    else
        skip("Ume P Gothic is not available, skipping some margin tests\n");

    if (is_font_installed("SimSun")) {
        test_margins_default("SimSun", GB2312_CHARSET);
        test_margins_default("SimSun", ANSI_CHARSET);
    }
    else
        skip("SimSun is not available, skipping some margin tests\n");
}

static void test_margins_font_change(void)
{
    DWORD margins, font_margins;
    HFONT hfont, hfont2;
    HWND hwEdit;
    LOGFONTA lf;

    if (!is_font_installed("Arial"))
    {
        skip("Arial not found - skipping font change margin tests\n");
        return;
    }

    hwEdit = create_child_editcontrol(0, 0);

    SetWindowPos(hwEdit, NULL, 10, 10, 1000, 100, SWP_NOZORDER | SWP_NOACTIVATE);

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Arial");
    lf.lfHeight = 16;
    lf.lfCharSet = GREEK_CHARSET; /* to avoid associated charset feature */
    hfont = CreateFontIndirectA(&lf);
    lf.lfHeight = 30;
    hfont2 = CreateFontIndirectA(&lf);

    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    font_margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(font_margins) != 0, "got %d\n", LOWORD(font_margins));
    ok(HIWORD(font_margins) != 0, "got %d\n", HIWORD(font_margins));

    /* With 'small' edit controls, test that the margin doesn't get set */
    SetWindowPos(hwEdit, NULL, 10, 10, 16, 100, SWP_NOZORDER | SWP_NOACTIVATE);
    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0,0));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 0, "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 0, "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,0));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 1, "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 0, "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,1));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 1, "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 1, "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(EC_USEFONTINFO,EC_USEFONTINFO));
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 1, "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 1, "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont2, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 1, "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 1, "got %d\n", HIWORD(margins));

    /* Above a certain size threshold then the margin is updated */
    SetWindowPos(hwEdit, NULL, 10, 10, 1000, 100, SWP_NOZORDER | SWP_NOACTIVATE);
    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,0));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == LOWORD(font_margins), "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == HIWORD(font_margins), "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,1));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == LOWORD(font_margins), "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == HIWORD(font_margins), "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(EC_USEFONTINFO,EC_USEFONTINFO));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == LOWORD(font_margins), "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == HIWORD(font_margins), "got %d\n", HIWORD(margins));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont2, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) != LOWORD(font_margins), "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) != HIWORD(font_margins), "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, WM_SETFONT, 0, 0);

    DeleteObject(hfont2);
    DeleteObject(hfont);
    destroy_child_editcontrol(hwEdit);

}

#define edit_pos_ok(expected, got, txt) edit_pos_ok_(__LINE__, expected, got, #txt)
static inline void edit_pos_ok_(unsigned line, DWORD expected, DWORD got, const char* txt)
{
    ok_(__FILE__, line)(expected == got, "wrong %s expected %ld got %ld\n", txt, expected, got);
}

#define check_pos(hwEdit, set_height, test_top, test_height, test_left) \
do { \
    RECT format_rect; \
    int left_margin; \
    set_client_height(hwEdit, set_height); \
    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM) &format_rect); \
    left_margin = LOWORD(SendMessageA(hwEdit, EM_GETMARGINS, 0, 0)); \
    edit_pos_ok(test_top, format_rect.top, vertical position); \
    edit_pos_ok((int)test_height, format_rect.bottom - format_rect.top, height); \
    edit_pos_ok(test_left, format_rect.left - left_margin, left); \
} while(0)

/* Starting with Win10, multiline edit controls can have any value for height.
 * Before, they always have a height which is a multiple of the font height.
 * So detect which model we're running the tests on, and adjust the expected
 * height accordingly.
 */

static BOOL old_height_model;

static DWORD compute_height(BOOL single_line, unsigned line_height, unsigned incr, unsigned limit)
{
    if (single_line) return line_height;
    if (incr >= limit) incr -= limit;
    return old_height_model ? (line_height + incr) - ((line_height + incr) % line_height) : line_height + incr;
}

static void get_font_and_metrics(HFONT *font, TEXTMETRICA *metrics)
{
    HDC dc;
    HFONT old_font;
    BOOL xb;

    /* Get a stock font for which we can determine the metrics */
    *font = GetStockObject(SYSTEM_FONT);
    ok (*font != NULL, "GetStockObject SYSTEM_FONT failed\n");
    dc = GetDC(NULL);
    ok (dc != NULL, "GetDC() failed\n");
    old_font = SelectObject(dc, font);
    xb = GetTextMetricsA(dc, metrics);
    ok (xb, "GetTextMetrics failed\n");
    SelectObject(dc, old_font);
    ReleaseDC(NULL, dc);
}

static BOOL determine_height_model(void)
{
    HWND hedit;
    HFONT font;
    TEXTMETRICA metrics;
    RECT format_rect;
    BOOL ret = TRUE;

    get_font_and_metrics(&font, &metrics);
    hedit = create_child_editcontrol(ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | WS_VISIBLE, 0);
    SendMessageA(hedit, WM_SETFONT, (WPARAM) font, FALSE);

    set_client_height(hedit, metrics.tmHeight + 1);
    SendMessageA(hedit, EM_GETRECT, 0, (LPARAM) &format_rect);

    old_height_model = TRUE;
    if (format_rect.bottom - format_rect.top == metrics.tmHeight + 1)
        old_height_model = FALSE;
    else
        ret = format_rect.bottom - format_rect.top == metrics.tmHeight;

    destroy_child_editcontrol(hedit);
    return ret;
}

static void test_text_position_style(DWORD style)
{
    HWND hedit;
    HFONT font;
    TEXTMETRICA metrics;
    INT b, b2, incr;
    BOOL single_line = !(style & ES_MULTILINE);

    get_font_and_metrics(&font, &metrics);
    b = GetSystemMetrics(SM_CYBORDER) + 1;
    b2 = 2 * b;

    /* Windows' edit control has some bugs in multi-line mode:
     * - Sometimes the format rectangle doesn't get updated
     *   (see workaround in set_client_height())
     * - If the height of the control is smaller than the height of a text
     *   line, the format rectangle is still as high as a text line
     *   (higher than the client rectangle) and the caret is not shown
     */

    /* Edit controls that are in a parent window */

    hedit = create_child_editcontrol(style | WS_VISIBLE, 0);
    SendMessageA(hedit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hedit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 0);
    for (incr = 0; incr < 4 * metrics.tmHeight; incr++)
        check_pos(hedit, metrics.tmHeight + incr, 0,
                  compute_height(single_line, metrics.tmHeight, incr, 0), 0);
    destroy_child_editcontrol(hedit);

    hedit = create_child_editcontrol(style | WS_BORDER | WS_VISIBLE, 0);
    SendMessageA(hedit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hedit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, b);
    for (incr = 0; incr < 4 * metrics.tmHeight; incr++)
        check_pos(hedit, metrics.tmHeight + incr, incr >= b2 ? b : 0,
                  compute_height(single_line, metrics.tmHeight, incr, b2), b);
    destroy_child_editcontrol(hedit);

    hedit = create_child_editcontrol(style | WS_VISIBLE, WS_EX_CLIENTEDGE);
    SendMessageA(hedit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hedit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 1);
    for (incr = 0; incr < 4 * metrics.tmHeight; incr++)
        check_pos(hedit, metrics.tmHeight + incr, incr >= 2 ? 1 : 0,
                  compute_height(single_line, metrics.tmHeight, incr, 2), 1);
    destroy_child_editcontrol(hedit);

    hedit = create_child_editcontrol(style | WS_BORDER | WS_VISIBLE, WS_EX_CLIENTEDGE);
    SendMessageA(hedit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hedit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 1);
    for (incr = 0; incr < 4 * metrics.tmHeight; incr++)
        check_pos(hedit, metrics.tmHeight + incr, incr >= 2 ? 1 : 0,
                  compute_height(single_line, metrics.tmHeight, incr, 2), 1);
    destroy_child_editcontrol(hedit);


    /* Edit controls that are popup windows */

    hedit = create_editcontrol(style | WS_POPUP, 0);
    SendMessageA(hedit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hedit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 0);
    for (incr = 0; incr < 4 * metrics.tmHeight; incr++)
        check_pos(hedit, metrics.tmHeight + incr, 0,
                  compute_height(single_line, metrics.tmHeight, incr, 0), 0);
    DestroyWindow(hedit);

    hedit = create_editcontrol(style | WS_POPUP | WS_BORDER, 0);
    SendMessageA(hedit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hedit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, b);
    for (incr = 0; incr < 4 * metrics.tmHeight; incr++)
        check_pos(hedit, metrics.tmHeight + incr, incr >= b2 ? b : 0,
                  compute_height(single_line, metrics.tmHeight, incr, b2), b);
    DestroyWindow(hedit);

    hedit = create_editcontrol(style | WS_POPUP, WS_EX_CLIENTEDGE);
    SendMessageA(hedit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hedit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 1);
    for (incr = 0; incr < 4 * metrics.tmHeight; incr++)
        check_pos(hedit, metrics.tmHeight + incr, incr >= 2 ? 1 : 0,
                  compute_height(single_line, metrics.tmHeight, incr, 2), 1);
    DestroyWindow(hedit);

    hedit = create_editcontrol(style | WS_POPUP | WS_BORDER, WS_EX_CLIENTEDGE);
    SendMessageA(hedit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hedit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 1);
    for (incr = 0; incr < 4 * metrics.tmHeight; incr++)
        check_pos(hedit, metrics.tmHeight + incr, incr >= 2 ? 1 : 0,
                  compute_height(single_line, metrics.tmHeight, incr, 2), 1);
    DestroyWindow(hedit);
}

static void test_text_position(void)
{
    ok (determine_height_model(), "Unable to guess the height model, forcing old one\n");
    trace("EDIT: Using %s model for height computation\n", old_height_model ? "old" : "new");

    trace("EDIT: Text position (Single line)\n");
    test_text_position_style(ES_AUTOHSCROLL | ES_AUTOVSCROLL);
    trace("EDIT: Text position (Multi line)\n");
    test_text_position_style(ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL);
}

static void test_espassword(void)
{
    HWND hwEdit;
    LONG r;
    char buffer[1024];
    const char* password = "secret";

    hwEdit = create_editcontrol(ES_PASSWORD, 0);
    r = get_edit_style(hwEdit);
    ok(r == ES_PASSWORD, "Wrong style expected ES_PASSWORD got: 0x%lx\n", r);
    /* set text */
    r = SendMessageA(hwEdit , WM_SETTEXT, 0, (LPARAM) password);
    ok(r == TRUE, "Expected: %d, got: %ld\n", TRUE, r);

    /* select all, cut (ctrl-x) */
    SendMessageA(hwEdit, EM_SETSEL, 0, -1);
    r = SendMessageA(hwEdit, WM_CHAR, 24, 0);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);

    /* get text */
    r = SendMessageA(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(r == strlen(password), "Expected: %s, got len %ld\n", password, r);
    ok(strcmp(buffer, password) == 0, "expected %s, got %s\n", password, buffer);

    r = open_clipboard(hwEdit);
    ok(r == TRUE, "expected %d, got %ld\n", TRUE, r);
    r = EmptyClipboard();
    ok(r == TRUE, "expected %d, got %ld\n", TRUE, r);
    r = CloseClipboard();
    ok(r == TRUE, "expected %d, got %ld\n", TRUE, r);

    /* select all, copy (ctrl-c) and paste (ctrl-v) */
    SendMessageA(hwEdit, EM_SETSEL, 0, -1);
    r = SendMessageA(hwEdit, WM_CHAR, 3, 0);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);
    r = SendMessageA(hwEdit, WM_CHAR, 22, 0);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(r == 0, "Expected: 0, got: %ld\n", r);
    ok(strcmp(buffer, "") == 0, "expected empty string, got %s\n", buffer);

    DestroyWindow(hwEdit);
}

static void test_undo(void)
{
    HWND hwEdit;
    LONG r;
    DWORD cpMin, cpMax;
    char buffer[1024];
    const char* text = "undo this";

    hwEdit = create_editcontrol(0, 0);
    r = get_edit_style(hwEdit);
    ok(0 == r, "Wrong style expected 0x%x got: 0x%lx\n", 0, r);

    /* set text */
    r = SendMessageA(hwEdit , WM_SETTEXT, 0, (LPARAM) text);
    ok(TRUE == r, "Expected: %d, got: %ld\n", TRUE, r);

    /* select all, */
    cpMin = cpMax = 0xdeadbeef;
    SendMessageA(hwEdit, EM_SETSEL, 0, -1);
    r = SendMessageA(hwEdit, EM_GETSEL, (WPARAM) &cpMin, (LPARAM) &cpMax);
    ok((strlen(text) << 16) == r, "Unexpected length %ld\n", r);
    ok(0 == cpMin, "Expected: %d, got %ld\n", 0, cpMin);
    ok(9 == cpMax, "Expected: %d, got %ld\n", 9, cpMax);

    /* cut (ctrl-x) */
    r = SendMessageA(hwEdit, WM_CHAR, 24, 0);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(0 == r, "Expected: %d, got len %ld\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected %s, got %s\n", "", buffer);

    /* undo (ctrl-z) */
    r = SendMessageA(hwEdit, WM_CHAR, 26, 0);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(strlen(text) == r, "Unexpected length %ld\n", r);
    ok(0 == strcmp(buffer, text), "expected %s, got %s\n", text, buffer);

    /* undo again (ctrl-z) */
    r = SendMessageA(hwEdit, WM_CHAR, 26, 0);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(r == 0, "Expected: %d, got len %ld\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected %s, got %s\n", "", buffer);

    DestroyWindow(hwEdit);
}

static void test_enter(void)
{
    char buffer[16];
    HWND hwEdit;
    LONG r;

    /* multiline */
    hwEdit = create_editcontrol(ES_MULTILINE, 0);
    r = get_edit_style(hwEdit);
    ok(ES_MULTILINE == r, "Wrong style expected ES_MULTILINE got: 0x%lx\n", r);

    /* set text */
    r = SendMessageA(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %ld\n", TRUE, r);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(2 == r, "Expected: %d, got len %ld\n", 2, r);
    ok(0 == strcmp(buffer, "\r\n"), "expected \"\\r\\n\", got \"%s\"\n", buffer);

    DestroyWindow (hwEdit);

    /* single line */
    hwEdit = create_editcontrol(0, 0);
    r = get_edit_style(hwEdit);
    ok(0 == r, "Wrong style expected 0x%x got: 0x%lx\n", 0, r);

    /* set text */
    r = SendMessageA(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %ld\n", TRUE, r);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(0 == r, "Expected: %d, got len %ld\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected \"\", got \"%s\"\n", buffer);

    DestroyWindow(hwEdit);

    /* single line with ES_WANTRETURN */
    hwEdit = create_editcontrol(ES_WANTRETURN, 0);
    r = get_edit_style(hwEdit);
    ok(ES_WANTRETURN == r, "Wrong style expected ES_WANTRETURN got: 0x%lx\n", r);

    /* set text */
    r = SendMessageA(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %ld\n", TRUE, r);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(0 == r, "Expected: %d, got len %ld\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected \"\", got \"%s\"\n", buffer);

    DestroyWindow(hwEdit);
}

static void test_tab(void)
{
    char buffer[16];
    HWND hwEdit;
    LONG r;

    /* multiline */
    hwEdit = create_editcontrol(ES_MULTILINE, 0);
    r = get_edit_style(hwEdit);
    ok(ES_MULTILINE == r, "Wrong style expected ES_MULTILINE got: 0x%lx\n", r);

    /* set text */
    r = SendMessageA(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %ld\n", TRUE, r);

    r = SendMessageA(hwEdit, WM_CHAR, VK_TAB, 0);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(1 == r, "Expected: %d, got len %ld\n", 1, r);
    ok(0 == strcmp(buffer, "\t"), "expected \"\\t\", got \"%s\"\n", buffer);

    DestroyWindow(hwEdit);

    /* single line */
    hwEdit = create_editcontrol(0, 0);
    r = get_edit_style(hwEdit);
    ok(0 == r, "Wrong style expected 0x%x got: 0x%lx\n", 0, r);

    /* set text */
    r = SendMessageA(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %ld\n", TRUE, r);

    r = SendMessageA(hwEdit, WM_CHAR, VK_TAB, 0);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(0 == r, "Expected: %d, got len %ld\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected \"\", got \"%s\"\n", buffer);

    DestroyWindow(hwEdit);
}

static void test_edit_dialog(void)
{
    int r;

    /* from bug 11841 */
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 0);
    ok(333 == r, "Expected %d, got %d\n", 333, r);
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 1);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* more tests for WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* more tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 6);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 7);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 8);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests with an editable edit control */
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 0);
    ok(333 == r, "Expected %d, got %d\n", 333, r);
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 1);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 6);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 7);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 8);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* multiple tab tests */
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 9);
    ok(22 == r, "Expected %d, got %d\n", 22, r);
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 10);
    ok(33 == r, "Expected %d, got %d\n", 33, r);
}

static void test_multi_edit_dialog(void)
{
    int r;

    /* test for multiple edit dialogs (bug 12319) */
    r = DialogBoxParamA(hinst, "MULTI_EDIT_DIALOG", NULL, multi_edit_dialog_proc, 0);
    ok(2222 == r, "Expected %d, got %d\n", 2222, r);
    r = DialogBoxParamA(hinst, "MULTI_EDIT_DIALOG", NULL, multi_edit_dialog_proc, 1);
    ok(1111 == r, "Expected %d, got %d\n", 1111, r);
    r = DialogBoxParamA(hinst, "MULTI_EDIT_DIALOG", NULL, multi_edit_dialog_proc, 2);
    ok(2222 == r, "Expected %d, got %d\n", 2222, r);
    r = DialogBoxParamA(hinst, "MULTI_EDIT_DIALOG", NULL, multi_edit_dialog_proc, 3);
    ok(11 == r, "Expected %d, got %d\n", 11, r);
}

static void test_wantreturn_edit_dialog(void)
{
    int r;

    /* tests for WM_KEYDOWN */
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 0);
    ok(333 == r, "Expected %d, got %d\n", 333, r);
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 1);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 6);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 7);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 8);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
}

static void test_singleline_wantreturn_edit_dialog(void)
{
    int r;

    /* tests for WM_KEYDOWN */
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 0);
    ok(222 == r, "Expected %d, got %d\n", 222, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 1);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 6);
    ok(222 == r, "Expected %d, got %d\n", 222, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 7);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 8);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN */
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 0);
    ok(222 == r, "Expected %d, got %d\n", 222, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 1);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 6);
    ok(222 == r, "Expected %d, got %d\n", 222, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 7);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 8);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
}

static int child_edit_wmkeydown_num_messages = 0;
static INT_PTR CALLBACK child_edit_wmkeydown_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_DESTROY:
        case WM_NCDESTROY:
            break;

        default:
            child_edit_wmkeydown_num_messages++;
            break;
    }

    return FALSE;
}

static void test_child_edit_wmkeydown(void)
{
    HWND hwEdit, hwParent;
    int r;

    hwEdit = create_child_editcontrol(0, 0);
    hwParent = GetParent(hwEdit);
    SetWindowLongPtrA(hwParent, GWLP_WNDPROC, (LONG_PTR)child_edit_wmkeydown_proc);
    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    ok(0 == child_edit_wmkeydown_num_messages, "expected 0, got %d\n", child_edit_wmkeydown_num_messages);
    destroy_child_editcontrol(hwEdit);
}

static BOOL got_en_setfocus = FALSE;
static BOOL got_wm_capturechanged = FALSE;
static LRESULT (CALLBACK *p_edit_proc)(HWND, UINT, WPARAM, LPARAM);

static LRESULT CALLBACK edit4_wnd_procA(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case EN_SETFOCUS:
                    got_en_setfocus = TRUE;
                    break;
            }
            break;
        case WM_CAPTURECHANGED:
            if (hWnd != (HWND)lParam)
            {
                got_wm_capturechanged = TRUE;
                EndMenu();
            }
            break;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

struct context_menu_messages
{
    unsigned int wm_command, em_setsel;
};

static struct context_menu_messages menu_messages;

static LRESULT CALLBACK child_edit_menu_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_ENTERIDLE:
        if (wParam == MSGF_MENU)
        {
            HWND hwndMenu = (HWND)lParam;
            MENUBARINFO mbi = { sizeof(mbi) };
            if (GetMenuBarInfo(hwndMenu, OBJID_CLIENT, 0, &mbi))
            {
                MENUITEMINFOA mii = { sizeof(MENUITEMINFOA), MIIM_STATE };
                if (GetMenuItemInfoA(mbi.hMenu, EM_SETSEL, FALSE, &mii))
                {
                    if (mii.fState & MFS_HILITE)
                    {
                        PostMessageA(hwnd, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                        PostMessageA(hwnd, WM_KEYUP, VK_RETURN, 0x1c0001);
                    }
                    else
                    {
                        PostMessageA(hwnd, WM_KEYDOWN, VK_DOWN, 0x500001);
                        PostMessageA(hwnd, WM_KEYUP, VK_DOWN, 0x500001);
                    }
                }
            }
        }
        break;
    case WM_COMMAND:
        menu_messages.wm_command++;
        break;
    case EM_SETSEL:
        menu_messages.em_setsel++;
        break;
    }
    return CallWindowProcA(p_edit_proc, hwnd, msg, wParam, lParam);
}

static void test_contextmenu(void)
{
    HWND hwndMain, hwndEdit;
    MSG msg;

    hwndMain = CreateWindowA(szEditTest4Class, "ET4", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                            0, 0, 200, 200, NULL, NULL, hinst, NULL);
    ok(hwndMain != NULL, "Failed to create control parent.\n");

    hwndEdit = CreateWindowA(WC_EDITA, NULL, WS_CHILD|WS_BORDER|WS_VISIBLE|ES_LEFT|ES_AUTOHSCROLL,
                           0, 0, 150, 50, /* important this not be 0 size. */
                           hwndMain, (HMENU) ID_EDITTEST2, hinst, NULL);
    ok(hwndEdit != NULL, "Failed to create Edit control.\n");

    SetFocus(NULL);
    SetCapture(hwndMain);
    SendMessageA(hwndEdit, WM_CONTEXTMENU, (WPARAM)hwndEdit, MAKEWORD(10, 10));
    ok(got_en_setfocus, "edit box didn't get focused\n");
    ok(got_wm_capturechanged, "main window capture did not change\n");

    DestroyWindow(hwndEdit);

    hwndEdit = CreateWindowA("EDIT", "Test Text",
                             WS_CHILD | WS_BORDER | WS_VISIBLE,
                             0, 0, 100, 100,
                             hwndMain, NULL, hinst, NULL);
    memset(&menu_messages, 0, sizeof(menu_messages));
    p_edit_proc = (void*)SetWindowLongPtrA(hwndEdit, GWLP_WNDPROC,
                                           (ULONG_PTR)child_edit_menu_proc);

    SetFocus(hwndEdit);
    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)"foo");
    SendMessageA(hwndEdit, WM_CONTEXTMENU, (WPARAM)hwndEdit, MAKEWORD(-1, -1));
    while (PeekMessageA(&msg, hwndEdit, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok(menu_messages.wm_command == 0,
       "Expected no WM_COMMAND messages, got %d\n", menu_messages.wm_command);
    ok(menu_messages.em_setsel == 1,
       "Expected 1 EM_SETSEL message, got %d\n", menu_messages.em_setsel);

    DestroyWindow(hwndEdit);
    DestroyWindow(hwndMain);
}

static BOOL register_classes(void)
{
    WNDCLASSA test2;
    WNDCLASSA test3;
    WNDCLASSA test4;
    WNDCLASSA text_position;
    WNDCLASSA wc;

    test2.style = 0;
    test2.lpfnWndProc = ET2_WndProc;
    test2.cbClsExtra = 0;
    test2.cbWndExtra = 0;
    test2.hInstance = hinst;
    test2.hIcon = NULL;
    test2.hCursor = LoadCursorA (NULL, (LPCSTR)IDC_ARROW);
    test2.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    test2.lpszMenuName = NULL;
    test2.lpszClassName = szEditTest2Class;
    if (!RegisterClassA(&test2)) return FALSE;

    test3.style = 0;
    test3.lpfnWndProc = edit3_wnd_procA;
    test3.cbClsExtra = 0;
    test3.cbWndExtra = 0;
    test3.hInstance = hinst;
    test3.hIcon = 0;
    test3.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    test3.hbrBackground = GetStockObject(WHITE_BRUSH);
    test3.lpszMenuName = NULL;
    test3.lpszClassName = szEditTest3Class;
    if (!RegisterClassA(&test3)) return FALSE;

    test4.style = 0;
    test4.lpfnWndProc = edit4_wnd_procA;
    test4.cbClsExtra = 0;
    test4.cbWndExtra = 0;
    test4.hInstance = hinst;
    test4.hIcon = NULL;
    test4.hCursor = LoadCursorA (NULL, (LPCSTR)IDC_ARROW);
    test4.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    test4.lpszMenuName = NULL;
    test4.lpszClassName = szEditTest4Class;
    if (!RegisterClassA(&test4)) return FALSE;

    text_position.style = CS_HREDRAW | CS_VREDRAW;
    text_position.cbClsExtra = 0;
    text_position.cbWndExtra = 0;
    text_position.hInstance = hinst;
    text_position.hIcon = NULL;
    text_position.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    text_position.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    text_position.lpszMenuName = NULL;
    text_position.lpszClassName = szEditTextPositionClass;
    text_position.lpfnWndProc = DefWindowProcA;
    if (!RegisterClassA(&text_position)) return FALSE;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = parent_wnd_proc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "ParentWnd";
    if (!RegisterClassA(&wc)) return FALSE;

    return TRUE;
}

static void UnregisterWindowClasses (void)
{
    UnregisterClassA(szEditTest2Class, hinst);
    UnregisterClassA(szEditTest3Class, hinst);
    UnregisterClassA(szEditTest4Class, hinst);
    UnregisterClassA(szEditTextPositionClass, hinst);
}

static void test_fontsize(void)
{
    HWND hwEdit;
    HFONT hfont;
    HDC hDC;
    LOGFONTA lf;
    LONG r;
    char szLocalString[MAXLEN];
    int dpi;

    hDC = GetDC(NULL);
    dpi = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(NULL, hDC);

    memset(&lf,0,sizeof(LOGFONTA));
    strcpy(lf.lfFaceName,"Arial");
    lf.lfHeight = -300; /* taller than the edit box */
    lf.lfWeight = 500;
    hfont = CreateFontIndirectA(&lf);

    trace("EDIT: Oversized font (Multi line)\n");
    hwEdit= CreateWindowA("EDIT", NULL, ES_MULTILINE|ES_AUTOHSCROLL,
                           0, 0, (150 * dpi) / 96, (50 * dpi) / 96, NULL, NULL,
                           hinst, NULL);

    SendMessageA(hwEdit,WM_SETFONT,(WPARAM)hfont,0);

    if (winetest_interactive)
        ShowWindow (hwEdit, SW_SHOW);

    r = SendMessageA(hwEdit, WM_CHAR, 'A', 1);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);
    r = SendMessageA(hwEdit, WM_CHAR, 'B', 1);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);
    r = SendMessageA(hwEdit, WM_CHAR, 'C', 1);
    ok(1 == r, "Expected: %d, got: %ld\n", 1, r);

    GetWindowTextA(hwEdit, szLocalString, MAXLEN);
    ok(strcmp(szLocalString, "ABC")==0,
       "Wrong contents of edit: %s\n", szLocalString);

    r = SendMessageA(hwEdit, EM_POSFROMCHAR,0,0);
    ok(r != -1,"EM_POSFROMCHAR failed index 0\n");
    r = SendMessageA(hwEdit, EM_POSFROMCHAR,1,0);
    ok(r != -1,"EM_POSFROMCHAR failed index 1\n");
    r = SendMessageA(hwEdit, EM_POSFROMCHAR,2,0);
    ok(r != -1,"EM_POSFROMCHAR failed index 2\n");
    r = SendMessageA(hwEdit, EM_POSFROMCHAR,3,0);
    ok(r == -1,"EM_POSFROMCHAR succeeded index 3\n");

    DestroyWindow (hwEdit);
    DeleteObject(hfont);
}

struct dialog_mode_messages
{
    int wm_getdefid, wm_close, wm_command, wm_nextdlgctl;
};

static struct dialog_mode_messages dm_messages;

static void zero_dm_messages(void)
{
    dm_messages.wm_command      = 0;
    dm_messages.wm_close        = 0;
    dm_messages.wm_getdefid     = 0;
    dm_messages.wm_nextdlgctl   = 0;
}

#define test_dm_messages(wmcommand, wmclose, wmgetdefid, wmnextdlgctl) \
    ok(dm_messages.wm_command == wmcommand, "expected %d WM_COMMAND messages, " \
    "got %d\n", wmcommand, dm_messages.wm_command); \
    ok(dm_messages.wm_close == wmclose, "expected %d WM_CLOSE messages, " \
    "got %d\n", wmclose, dm_messages.wm_close); \
    ok(dm_messages.wm_getdefid == wmgetdefid, "expected %d WM_GETDIFID messages, " \
    "got %d\n", wmgetdefid, dm_messages.wm_getdefid);\
    ok(dm_messages.wm_nextdlgctl == wmnextdlgctl, "expected %d WM_NEXTDLGCTL messages, " \
    "got %d\n", wmnextdlgctl, dm_messages.wm_nextdlgctl)

static LRESULT CALLBACK dialog_mode_wnd_proc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
        case WM_COMMAND:
            dm_messages.wm_command++;
            break;
        case DM_GETDEFID:
            dm_messages.wm_getdefid++;
            return MAKELONG(ID_EDITTESTDBUTTON, DC_HASDEFID);
        case WM_NEXTDLGCTL:
            dm_messages.wm_nextdlgctl++;
            break;
        case WM_CLOSE:
            dm_messages.wm_close++;
            break;
    }

    return DefWindowProcA(hwnd, iMsg, wParam, lParam);
}

static void test_dialogmode(void)
{
    HWND hwEdit, hwParent, hwButton;
    MSG msg= {0};
    int len, r;
    hwEdit = create_child_editcontrol(ES_MULTILINE, 0);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    len = SendMessageA(hwEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(11 == len, "expected 11, got %d\n", len);

    r = SendMessageA(hwEdit, WM_GETDLGCODE, 0, 0);
    ok(0x8d == r, "expected 0x8d, got 0x%x\n", r);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    len = SendMessageA(hwEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(13 == len, "expected 13, got %d\n", len);

    r = SendMessageA(hwEdit, WM_GETDLGCODE, 0, (LPARAM)&msg);
    ok(0x8d == r, "expected 0x8d, got 0x%x\n", r);
    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    len = SendMessageA(hwEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(13 == len, "expected 13, got %d\n", len);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    len = SendMessageA(hwEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(13 == len, "expected 13, got %d\n", len);

    destroy_child_editcontrol(hwEdit);

    hwEdit = create_editcontrol(ES_MULTILINE, 0);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    len = SendMessageA(hwEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(11 == len, "expected 11, got %d\n", len);

    msg.hwnd = hwEdit;
    msg.message = WM_KEYDOWN;
    msg.wParam = VK_BACK;
    msg.lParam = 0xe0001;
    r = SendMessageA(hwEdit, WM_GETDLGCODE, VK_BACK, (LPARAM)&msg);
    ok(0x8d == r, "expected 0x8d, got 0x%x\n", r);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    len = SendMessageA(hwEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(11 == len, "expected 11, got %d\n", len);

    DestroyWindow(hwEdit);

    hwEdit = create_child_editcontrol(0, 0);
    hwParent = GetParent(hwEdit);
    SetWindowLongPtrA(hwParent, GWLP_WNDPROC, (LONG_PTR)dialog_mode_wnd_proc);

    zero_dm_messages();
    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 0, 0);
    zero_dm_messages();

    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 0, 0);
    zero_dm_messages();

    msg.hwnd = hwEdit;
    msg.message = WM_KEYDOWN;
    msg.wParam = VK_TAB;
    msg.lParam = 0xf0001;
    r = SendMessageA(hwEdit, WM_GETDLGCODE, VK_TAB, (LPARAM)&msg);
    ok(0x89 == r, "expected 0x89, got 0x%x\n", r);
    test_dm_messages(0, 0, 0, 0);
    zero_dm_messages();

    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 0, 0);
    zero_dm_messages();

    destroy_child_editcontrol(hwEdit);

    hwEdit = create_child_editcontrol(ES_MULTILINE, 0);
    hwParent = GetParent(hwEdit);
    SetWindowLongPtrA(hwParent, GWLP_WNDPROC, (LONG_PTR)dialog_mode_wnd_proc);

    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 0, 0);
    zero_dm_messages();

    msg.hwnd = hwEdit;
    msg.message = WM_KEYDOWN;
    msg.wParam = VK_ESCAPE;
    msg.lParam = 0x10001;
    r = SendMessageA(hwEdit, WM_GETDLGCODE, VK_ESCAPE, (LPARAM)&msg);
    ok(0x8d == r, "expected 0x8d, got 0x%x\n", r);
    test_dm_messages(0, 0, 0, 0);
    zero_dm_messages();

    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 0, 0);
    zero_dm_messages();

    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 0, 1);
    zero_dm_messages();

    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 1, 0);
    zero_dm_messages();

    hwButton = CreateWindowA("BUTTON", "OK", WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
        100, 100, 50, 20, hwParent, (HMENU)ID_EDITTESTDBUTTON, hinst, NULL);
    ok(hwButton!=NULL, "CreateWindow failed with error code %ld\n", GetLastError());

    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 1, 1);
    zero_dm_messages();

    DestroyWindow(hwButton);
    destroy_child_editcontrol(hwEdit);
}

static void test_EM_GETHANDLE(void)
{
    static const WCHAR str1W[] = {'1','1','1','1','+','1','1','1','1','+','1','1','1','1','#',0};
    static const WCHAR str2W[] = {'2','2','2','2','-','2','2','2','2','-','2','2','2','2','-','2','2','2','2','#',0};
    static const char str0[] = "untouched";
    static const char str1[] = "1111+1111+1111#";
    static const char str1_1[] = "2111+1111+1111#";
    static const char str2[] = "2222-2222-2222-2222#";
    static const char str3[] = "3333*3333*3333*3333*3333#";
    CHAR    current[42];
    HWND    hEdit;
    HLOCAL  hmem;
    HLOCAL  hmem2;
    HLOCAL  halloc;
    WCHAR  *buffer;
    int     len;
    int     r;

    trace("EDIT: EM_GETHANDLE\n");

    /* EM_GETHANDLE is not supported for a single line edit control */
    hEdit = create_editcontrol(WS_BORDER, 0);
    ok(hEdit != NULL, "got %p (expected != NULL)\n", hEdit);

    hmem = (HGLOBAL) SendMessageA(hEdit, EM_GETHANDLE, 0, 0);
    ok(hmem == NULL, "got %p (expected NULL)\n", hmem);
    DestroyWindow(hEdit);

    /* EM_GETHANDLE needs a multiline edit control */
    hEdit = create_editcontrol(WS_BORDER | ES_MULTILINE, 0);
    ok(hEdit != NULL, "got %p (expected != NULL)\n", hEdit);

    /* set some text */
    r = SendMessageA(hEdit, WM_SETTEXT, 0, (LPARAM)str1);
    len = SendMessageA(hEdit, WM_GETTEXTLENGTH, 0, 0);
    ok((r == 1) && (len == lstrlenA(str1)), "got %d and %d (expected 1 and %d)\n", r, len, lstrlenA(str1));

    lstrcpyA(current, str0);
    r = SendMessageA(hEdit, WM_GETTEXT, sizeof(current), (LPARAM)current);
    ok((r == lstrlenA(str1)) && !lstrcmpA(current, str1),
        "got %d and \"%s\" (expected %d and \"%s\")\n", r, current, lstrlenA(str1), str1);

    hmem = (HGLOBAL) SendMessageA(hEdit, EM_GETHANDLE, 0, 0);
    ok(hmem != NULL, "got %p (expected != NULL)\n", hmem);
    /* The buffer belongs to the app now. According to MSDN, the app has to LocalFree the
       buffer, LocalAlloc a new buffer and pass it to the edit control with EM_SETHANDLE. */

    buffer = LocalLock(hmem);
    ok(buffer != NULL, "got %p (expected != NULL)\n", buffer);
    len = lstrlenW(buffer);
    ok(len == lstrlenW(str1W) && !lstrcmpW(buffer, str1W), "Unexpected buffer contents %s, length %d.\n",
        wine_dbgstr_w(buffer), len);
    LocalUnlock(hmem);

    /* See if WM_GETTEXTLENGTH/WM_GETTEXT still work. */
    len = SendMessageA(hEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(len == lstrlenA(str1), "Unexpected text length %d.\n", len);

    lstrcpyA(current, str0);
    r = SendMessageA(hEdit, WM_GETTEXT, sizeof(current), (LPARAM)current);
    ok((r == lstrlenA(str1)) && !lstrcmpA(current, str1),
        "Unexpected retval %d and text \"%s\" (expected %d and \"%s\")\n", r, current, lstrlenA(str1), str1);

    /* Application altered buffer contents, see if WM_GETTEXTLENGTH/WM_GETTEXT pick that up. */
    buffer = LocalLock(hmem);
    ok(buffer != NULL, "got %p (expected != NULL)\n", buffer);
    buffer[0] = '2';
    LocalUnlock(hmem);

    len = SendMessageA(hEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(len == lstrlenA(str1_1), "Unexpected text length %d.\n", len);

    lstrcpyA(current, str0);
    r = SendMessageA(hEdit, WM_GETTEXT, sizeof(current), (LPARAM)current);
    ok(r == lstrlenA(str1_1) && !lstrcmpA(current, str1_1),
        "Unexpected retval %d and text \"%s\" (expected %d and \"%s\")\n", r, current, lstrlenA(str1_1), str1_1);

    /* See if WM_SETTEXT/EM_REPLACESEL work. */
    r = SendMessageA(hEdit, WM_SETTEXT, 0, (LPARAM)str1);
    ok(r, "Failed to set text.\n");

    buffer = LocalLock(hmem);
    ok(buffer != NULL && buffer[0] == '1', "Unexpected buffer contents\n");
    LocalUnlock(hmem);

    r = SendMessageA(hEdit, EM_REPLACESEL, 0, (LPARAM)str1_1);
    ok(r, "Failed to replace selection.\n");

    buffer = LocalLock(hmem);
    ok(buffer != NULL && buffer[0] == '2', "Unexpected buffer contents\n");
    LocalUnlock(hmem);

    /* use LocalAlloc first to get a different handle */
    halloc = LocalAlloc(LMEM_MOVEABLE, 42);
    ok(halloc != NULL, "got %p (expected != NULL)\n", halloc);
    /* prepare our new memory */
    buffer = LocalLock(halloc);
    ok(buffer != NULL, "got %p (expected != NULL)\n", buffer);
    lstrcpyW(buffer, str2W);
    LocalUnlock(halloc);

    /* LocalFree the old memory handle before EM_SETHANDLE the new handle */
    LocalFree(hmem);
    /* use LocalAlloc after the LocalFree to likely consume the handle */
    hmem2 = LocalAlloc(LMEM_MOVEABLE, 42);
    ok(hmem2 != NULL, "got %p (expected != NULL)\n", hmem2);

    SendMessageA(hEdit, EM_SETHANDLE, (WPARAM)halloc, 0);

    len = SendMessageA(hEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(len == lstrlenA(str2), "got %d (expected %d)\n", len, lstrlenA(str2));

    lstrcpyA(current, str0);
    r = SendMessageA(hEdit, WM_GETTEXT, sizeof(current), (LPARAM)current);
    ok(r == lstrlenA(str2) && !lstrcmpA(current, str2),
        "got %d and \"%s\" (expected %d and \"%s\")\n", r, current, lstrlenA(str2), str2);

    /* set a different text */
    r = SendMessageA(hEdit, WM_SETTEXT, 0, (LPARAM)str3);
    len = SendMessageA(hEdit, WM_GETTEXTLENGTH, 0, 0);
    ok((r == 1) && (len == lstrlenA(str3)), "got %d and %d (expected 1 and %d)\n", r, len, lstrlenA(str3));

    lstrcpyA(current, str0);
    r = SendMessageA(hEdit, WM_GETTEXT, sizeof(current), (LPARAM)current);
    ok((r == lstrlenA(str3)) && !lstrcmpA(current, str3),
        "got %d and \"%s\" (expected %d and \"%s\")\n", r, current, lstrlenA(str3), str3);

    LocalFree(hmem2);
    DestroyWindow(hEdit);

    /* Some apps have bugs ... */
    hEdit = create_editcontrol(WS_BORDER | ES_MULTILINE, 0);

    /* set some text */
    r = SendMessageA(hEdit, WM_SETTEXT, 0, (LPARAM)str1);
    len = SendMessageA(hEdit, WM_GETTEXTLENGTH, 0, 0);
    ok((r == 1) && (len == lstrlenA(str1)), "got %d and %d (expected 1 and %d)\n", r, len, lstrlenA(str1));

    /* everything is normal up to EM_GETHANDLE */
    hmem = (HGLOBAL) SendMessageA(hEdit, EM_GETHANDLE, 0, 0);
    /* Some messages still work while other messages fail.
       After LocalFree the memory handle, messages can crash the app */

    /* A buggy editor used EM_GETHANDLE twice */
    hmem2 = (HGLOBAL) SendMessageA(hEdit, EM_GETHANDLE, 0, 0);
    ok(hmem2 == hmem, "got %p (expected %p)\n", hmem2, hmem);

    /* Let the edit control free the memory handle */
    SendMessageA(hEdit, EM_SETHANDLE, (WPARAM)hmem2, 0);

    DestroyWindow(hEdit);
}

/* In Windows 10+, the WM_PASTE message isn't always successful.
 * So retry in case of failure.
 */
#define check_paste(a, b) _check_paste(__LINE__, (a), (b))
static void _check_paste(unsigned int line, HWND hedit, const char *content)
{
    /* only retry on windows platform */
    int tries = strcmp(winetest_platform, "wine") ? 3 : 1;
    int len = 0;

    SendMessageA(hedit, WM_SETTEXT, 0, (LPARAM)"");
    do
    {
        SendMessageA(hedit, WM_PASTE, 0, 0);
        if ((len = SendMessageA(hedit, WM_GETTEXTLENGTH, 0, 0))) break;
        Sleep(1);
    } while (--tries > 0);
    ok_(__FILE__, line)(len == strlen(content), "Unexpected len %u in edit\n", len);
}

static void test_paste(void)
{
    static const char *str = "this is a simple text";
    static const char *str2 = "first line\r\nsecond line";
    HWND hEdit, hMultilineEdit;
    HANDLE hmem, hmem_ret;
    char *buffer;
    int r;

    hEdit = create_editcontrol(ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    hMultilineEdit = create_editcontrol(ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE, 0);

    /* Prepare clipboard data with simple text */
    hmem = GlobalAlloc(GMEM_MOVEABLE, 255);
    ok(hmem != NULL, "got %p (expected != NULL)\n", hmem);
    buffer = GlobalLock(hmem);
    ok(buffer != NULL, "got %p (expected != NULL)\n", buffer);
    strcpy(buffer, str);
    GlobalUnlock(hmem);

    r = open_clipboard(hEdit);
    ok(r == TRUE, "expected %d, got %d\n", TRUE, r);
    r = EmptyClipboard();
    ok(r == TRUE, "expected %d, got %d\n", TRUE, r);
    hmem_ret = SetClipboardData(CF_TEXT, hmem);
    ok(hmem_ret == hmem, "expected %p, got %p\n", hmem, hmem_ret);
    r = CloseClipboard();
    ok(r == TRUE, "expected %d, got %d\n", TRUE, r);

    /* Paste single line */
    check_paste(hEdit, str);

    /* Prepare clipboard data with multiline text */
    hmem = GlobalAlloc(GMEM_MOVEABLE, 255);
    ok(hmem != NULL, "got %p (expected != NULL)\n", hmem);
    buffer = GlobalLock(hmem);
    ok(buffer != NULL, "got %p (expected != NULL)\n", buffer);
    strcpy(buffer, str2);
    GlobalUnlock(hmem);

    r = open_clipboard(hEdit);
    ok(r == TRUE, "expected %d, got %d\n", TRUE, r);
    r = EmptyClipboard();
    ok(r == TRUE, "expected %d, got %d\n", TRUE, r);
    hmem_ret = SetClipboardData(CF_TEXT, hmem);
    ok(hmem_ret == hmem, "expected %p, got %p\n", hmem, hmem_ret);
    r = CloseClipboard();
    ok(r == TRUE, "expected %d, got %d\n", TRUE, r);

    /* Paste multiline text in singleline edit - should be cut */
    SendMessageA(hEdit, WM_SETTEXT, 0, (LPARAM)"");
    check_paste(hEdit, "first line");

    /* Paste multiline text in multiline edit */
    check_paste(hMultilineEdit, str2);

    /* Cleanup */
    DestroyWindow(hEdit);
    DestroyWindow(hMultilineEdit);
}

static void test_EM_GETLINE(void)
{
    HWND hwnd[2];
    int i;

    hwnd[0] = create_editcontrol(ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    hwnd[1] = create_editcontrolW(ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);

    for (i = 0; i < ARRAY_SIZE(hwnd); i++)
    {
        static const WCHAR strW[] = {'t','e','x','t',0};
        static const char *str = "text";
        WCHAR buffW[16];
        char buff[16];
        int r;

        ok(IsWindowUnicode(hwnd[i]), "Expected unicode window.\n");

        SendMessageA(hwnd[i], WM_SETTEXT, 0, (LPARAM)str);

        memset(buff, 0, sizeof(buff));
        *(WORD *)buff = sizeof(buff);
        r = SendMessageA(hwnd[i], EM_GETLINE, 0, (LPARAM)buff);
        ok(r == strlen(str), "Failed to get a line %d.\n", r);
        ok(!strcmp(buff, str), "Unexpected line data %s.\n", buff);

        memset(buff, 0, sizeof(buff));
        *(WORD *)buff = sizeof(buff);
        r = SendMessageA(hwnd[i], EM_GETLINE, 1, (LPARAM)buff);
        ok(r == strlen(str), "Failed to get a line %d.\n", r);
        ok(!strcmp(buff, str), "Unexpected line data %s.\n", buff);

        memset(buffW, 0, sizeof(buffW));
        *(WORD *)buffW = ARRAY_SIZE(buffW);
        r = SendMessageW(hwnd[i], EM_GETLINE, 0, (LPARAM)buffW);
        ok(r == lstrlenW(strW), "Failed to get a line %d.\n", r);
        ok(!lstrcmpW(buffW, strW), "Unexpected line data %s.\n", wine_dbgstr_w(buffW));

        memset(buffW, 0, sizeof(buffW));
        *(WORD *)buffW = ARRAY_SIZE(buffW);
        r = SendMessageW(hwnd[i], EM_GETLINE, 1, (LPARAM)buffW);
        ok(r == lstrlenW(strW), "Failed to get a line %d.\n", r);
        ok(!lstrcmpW(buffW, strW), "Unexpected line data %s.\n", wine_dbgstr_w(buffW));

        DestroyWindow(hwnd[i]);
    }
}

static int CALLBACK test_wordbreak_procA(char *text, int current, int length, int code)
{
    return -1;
}

static void test_wordbreak_proc(void)
{
    EDITWORDBREAKPROCA proc;
    LRESULT ret;
    HWND hwnd;

    hwnd = create_editcontrol(ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);

    proc = (void *)SendMessageA(hwnd, EM_GETWORDBREAKPROC, 0, 0);
    ok(proc == NULL, "Unexpected wordbreak proc %p.\n", proc);

    ret = SendMessageA(hwnd, EM_SETWORDBREAKPROC, 0, (LPARAM)test_wordbreak_procA);
    ok(ret == 1, "Unexpected return value %Id.\n", ret);

    proc = (void *)SendMessageA(hwnd, EM_GETWORDBREAKPROC, 0, 0);
    ok(proc == test_wordbreak_procA, "Unexpected wordbreak proc %p.\n", proc);

    ret = SendMessageA(hwnd, EM_SETWORDBREAKPROC, 0, 0);
    ok(ret == 1, "Unexpected return value %Id.\n", ret);

    proc = (void *)SendMessageA(hwnd, EM_GETWORDBREAKPROC, 0, 0);
    ok(proc == NULL, "Unexpected wordbreak proc %p.\n", proc);

    DestroyWindow(hwnd);
}

static const struct message setfocus_combined_seq[] =
{
    { WM_KILLFOCUS,    sent|id,            0, 0,                      PARENT_ID },
    { WM_SETFOCUS,     sent|id,            0, 0,                      EDIT_ID   },
    { WM_COMMAND,      sent|wparam|id, MAKEWPARAM(1, EN_SETFOCUS), 0, PARENT_ID },
    { WM_PAINT,        sent|id,            0, 0,                      EDIT_ID   },
    { WM_NCPAINT,      sent|id|defwinproc|optional, 0, 0,             EDIT_ID   },
    { WM_ERASEBKGND,   sent|id|defwinproc|optional, 0, 0,             EDIT_ID   },
    { 0 }
};

static const struct message killfocus_combined_seq[] =
{
    { WM_KILLFOCUS,    sent|id,            0, 0,                       EDIT_ID   },
    { WM_COMMAND,      sent|wparam|id, MAKEWPARAM(1, EN_KILLFOCUS), 0, PARENT_ID },
    { WM_SETFOCUS,     sent|id,            0, 0,                       PARENT_ID },
    { WM_PAINT,        sent|id,            0, 0,                       EDIT_ID   },
    { WM_NCPAINT,      sent|id|defwinproc|optional, 0, 0,              EDIT_ID   },
    { 0 }
};

static const struct message setfocus_sent_only_combined_seq[] =
{
    { WM_KILLFOCUS,    sent|id,            0, 0,                      PARENT_ID },
    { WM_SETFOCUS,     sent|id,            0, 0,                      EDIT_ID   },
    { WM_COMMAND,      sent|wparam|id, MAKEWPARAM(1, EN_SETFOCUS), 0, PARENT_ID },
    { 0 }
};

static const struct message killfocus_sent_only_combined_seq[] =
{
    { WM_KILLFOCUS,    sent|id,            0, 0,                       EDIT_ID   },
    { WM_COMMAND,      sent|wparam|id, MAKEWPARAM(1, EN_KILLFOCUS), 0, PARENT_ID },
    { WM_SETFOCUS,     sent|id,            0, 0,                       PARENT_ID },
    { 0 }
};

static void test_cue_banner(void)
{
    HWND hwnd_edit;
    BOOL ret;
    static WCHAR getcuetestW[5] = {'T',0};
    static const WCHAR testcmp1W[] = {'T','e','s','t',0};
    static const WCHAR testcmp2W[] = {'T','e','s',0};

    hwnd_edit = create_editcontrolW(ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);

    ret = SendMessageW(hwnd_edit, EM_GETCUEBANNER, (WPARAM)getcuetestW, 5);
    if (getcuetestW[0])
    {
        win_skip("skipping for Win XP and 2003 Server.\n");
        DestroyWindow(hwnd_edit);
        return;
    }
    ok(!getcuetestW[0], "First char is %c\n", getcuetestW[0]);
    ok(ret == FALSE, "EM_GETCUEBANNER should have returned FALSE.\n");

    lstrcpyW(getcuetestW, testcmp1W);
    ret = SendMessageW(hwnd_edit, EM_GETCUEBANNER, (WPARAM)getcuetestW, 0);
    ok(lstrcmpW(getcuetestW, testcmp1W) == 0, "String was %s.\n", wine_dbgstr_w(getcuetestW));
    ok(ret == FALSE, "EM_GETCUEBANNER should have returned FALSE.\n");

    ret = SendMessageW(hwnd_edit, EM_GETCUEBANNER, 0, 0);
    ok(ret == FALSE, "EM_GETCUEBANNER should have returned FALSE.\n");

    ret = SendMessageW(hwnd_edit, EM_SETCUEBANNER, 0, 0);
    ok(ret == FALSE, "EM_SETCUEBANNER should have returned FALSE.\n");

    ret = SendMessageW(hwnd_edit, EM_GETCUEBANNER, 0, 0);
    ok(ret == FALSE, "EM_GETCUEBANNER should have returned FALSE.\n");

    lstrcpyW(getcuetestW, testcmp1W);
    ret = SendMessageW(hwnd_edit, EM_SETCUEBANNER, 0, (LPARAM)getcuetestW);
    ok(ret == TRUE, "EM_SETCUEBANNER should have returned TRUE.\n");

    ret = SendMessageW(hwnd_edit, EM_GETCUEBANNER, 0, 5);
    ok(ret == TRUE, "EM_GETCUEBANNER should have returned TRUE.\n");

    ret = SendMessageW(hwnd_edit, EM_GETCUEBANNER, (WPARAM)getcuetestW, 5);
    ok(ret == TRUE, "EM_GETCUEBANNER should have returned TRUE.\n");
    ok(lstrcmpW(getcuetestW, testcmp1W) == 0, "EM_GETCUEBANNER returned string %s.\n", wine_dbgstr_w(getcuetestW));

    ret = SendMessageW(hwnd_edit, EM_SETCUEBANNER, 0, (LPARAM)L"");
    ok(ret == TRUE, "EM_SETCUEBANNER should have returned TRUE.\n");

    ret = SendMessageW(hwnd_edit, EM_GETCUEBANNER, (WPARAM)getcuetestW, 5);
    ok(ret == TRUE, "EM_GETCUEBANNER should have returned TRUE.\n");
    ok(!getcuetestW[0], "EM_GETCUEBANNER returned string %s.\n", wine_dbgstr_w(getcuetestW));

    /* EM_GETCUEBANNER's buffer size includes null char */
    ret = SendMessageW(hwnd_edit, EM_SETCUEBANNER, 0, (LPARAM)testcmp1W);
    ok(ret == TRUE, "EM_SETCUEBANNER should have returned TRUE.\n");
    memset(getcuetestW, 0, lstrlenW(testcmp1W)*sizeof(WCHAR));
    ret = SendMessageW(hwnd_edit, EM_GETCUEBANNER, (WPARAM)getcuetestW, (LPARAM)lstrlenW(testcmp1W)+1);
    ok(ret == TRUE, "EM_GETCUEBANNER should have returned TRUE.\n");
    ok(lstrcmpW(getcuetestW, testcmp1W) == 0, "EM_GETCUEBANNER returned string %s.\n", wine_dbgstr_w(getcuetestW));
    memset(getcuetestW, 0, lstrlenW(testcmp1W)*sizeof(WCHAR));
    ret = SendMessageW(hwnd_edit, EM_GETCUEBANNER, (WPARAM)getcuetestW, (LPARAM)lstrlenW(testcmp1W));
    ok(lstrcmpW(getcuetestW, testcmp2W) == 0, "EM_GETCUEBANNER returned string %s.\n", wine_dbgstr_w(getcuetestW));
    DestroyWindow(hwnd_edit);

    /* setting cue banner fails for multi-line edit controls */
    hwnd_edit = create_editcontrolW(ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE, 0);
    lstrcpyW(getcuetestW, testcmp1W);
    ret = SendMessageW(hwnd_edit, EM_GETCUEBANNER, (WPARAM)getcuetestW, 5);
    ok(ret == FALSE, "EM_SETCUEBANNER.\n");
    ok(lstrcmpW(getcuetestW, testcmp1W) == 0, "String was %s.\n", wine_dbgstr_w(getcuetestW));
    ret = SendMessageW(hwnd_edit, EM_SETCUEBANNER, 0, (LPARAM)getcuetestW);
    ok(ret == FALSE, "EM_SETCUEBANNER.\n");

    DestroyWindow(hwnd_edit);
}

static void test_change_focus(void)
{
    HWND hwnd, parent_wnd;
    WNDPROC oldproc;
    MSG msg;
    POINT orig_pos;

    GetCursorPos(&orig_pos);

    parent_wnd = CreateWindowA("ParentWnd", "", WS_OVERLAPPEDWINDOW,
            0, 0, 200, 200, NULL, NULL, GetModuleHandleA(NULL), NULL);
    ok(parent_wnd != NULL, "Failed to create control parent.\n");
    SetWindowPos(parent_wnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ShowWindow(parent_wnd, SW_SHOW);

    hwnd = CreateWindowExA(0, WC_EDITA, "Test", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100,
            parent_wnd, (HMENU)1, GetModuleHandleA(NULL), NULL);
    ok(hwnd != NULL, "Failed to create Edit control.\n");

    oldproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)edit_subclass_proc);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)oldproc);

    SetCursorPos(400, 400);
    flush_events();

    SetFocus(parent_wnd);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    SetFocus(hwnd);
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok_sequence(sequences, COMBINED_SEQ_INDEX, setfocus_combined_seq, "Set focus", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    SetFocus(parent_wnd);
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok_sequence(sequences, COMBINED_SEQ_INDEX, killfocus_combined_seq, "Kill focus", FALSE);

    /* Test message sequences without waiting for posted messages */
    SetFocus(parent_wnd);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    SetFocus(hwnd);
    ok_sequence(sequences, COMBINED_SEQ_INDEX, setfocus_sent_only_combined_seq,
                "Set focus sent only", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    SetFocus(parent_wnd);
    ok_sequence(sequences, COMBINED_SEQ_INDEX, killfocus_sent_only_combined_seq,
                "Kill focus sent only", FALSE);

    SetCursorPos(orig_pos.x, orig_pos.y);

    DestroyWindow(hwnd);
}

static const struct message wm_ime_composition_seq[] =
{
    {WM_IME_STARTCOMPOSITION, sent},
    {WM_IME_COMPOSITION, sent | wparam, 'W'},
    {WM_IME_CHAR, sent | wparam | defwinproc, 'W'},
    {WM_IME_CHAR, sent | wparam | defwinproc, 'i'},
    {WM_IME_CHAR, sent | wparam | defwinproc, 'n'},
    {WM_IME_CHAR, sent | wparam | defwinproc, 'e'},
    {WM_IME_ENDCOMPOSITION, sent},
    {WM_CHAR, sent | wparam, 'W'},
    {WM_CHAR, sent | wparam, 'i'},
    {WM_CHAR, sent | wparam, 'n'},
    {WM_CHAR, sent | wparam, 'e'},
    {0}
};

static const struct message wm_ime_composition_korean_seq[] =
{
    {WM_IME_ENDCOMPOSITION, sent},
    {WM_IME_COMPOSITION, sent | wparam, 'W'},
    {WM_IME_CHAR, sent | wparam | defwinproc, 'W'},
    {WM_IME_CHAR, sent | wparam | defwinproc, 'i'},
    {WM_IME_CHAR, sent | wparam | defwinproc, 'n'},
    {WM_IME_CHAR, sent | wparam | defwinproc, 'e'},
    {WM_CHAR, sent | wparam, 'W'},
    {WM_CHAR, sent | wparam, 'i'},
    {WM_CHAR, sent | wparam, 'n'},
    {WM_CHAR, sent | wparam, 'e'},
    {0}
};

static const struct message wm_ime_char_seq[] =
{
    {WM_IME_CHAR, sent | wparam, '0'},
    {WM_CHAR, sent | wparam, '0'},
    {0}
};

static const struct message eimes_getcompstratonce_seq[] =
{
    {WM_IME_STARTCOMPOSITION, sent},
    {WM_IME_COMPOSITION, sent | wparam, 'W'},
    {WM_IME_ENDCOMPOSITION, sent},
    {0}
};

static LRESULT CALLBACK edit_ime_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    struct message msg = {0};
    LRESULT ret;

    msg.message = message;
    msg.flags = sent | wparam;
    if (defwndproc_counter)
        msg.flags |= defwinproc;
    msg.wParam = wParam;

    if (message < 0xc000 &&
        message != WM_GETTEXTLENGTH &&
        message != WM_GETTEXT &&
        message != WM_GETFONT &&
        message != WM_GETICON &&
        message != WM_IME_SETCONTEXT &&
        message != WM_IME_NOTIFY &&
        message != WM_CTLCOLOREDIT &&
        message != WM_PAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCHITTEST &&
        message != WM_SETCURSOR &&
        message != WM_MOUSEMOVE &&
        message != WM_MOUSEACTIVATE &&
        message != WM_KEYUP &&
        (message < EM_GETSEL || message > EM_GETIMESTATUS))
    {
        add_message(sequences, COMBINED_SEQ_INDEX, &msg);
    }

    defwndproc_counter++;
    if (IsWindowUnicode(hwnd))
        ret = CallWindowProcW(oldproc, hwnd, message, wParam, lParam);
    else
        ret = CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static void test_ime(void)
{
    static const HKL korean_hkl = (HKL)0x04120412;
    WNDPROC old_proc;
    LRESULT lr;
    HIMC himc;
    HWND hwnd;
    BOOL ret;
    HKL hkl;
    MSG msg;

    hkl = GetKeyboardLayout(0);

    hwnd = create_editcontrol(WS_POPUP | WS_VISIBLE, 0);

    /* Test EM_{GET|SET}IMESTATUS */
    lr = SendMessageA(hwnd, EM_GETIMESTATUS, EMSIS_COMPOSITIONSTRING, 0);
    ok(lr == 0, "Got unexpected lr %#Ix.\n", lr);

    /* Note that EM_SETIMESTATUS always return 1, which is contrary to what MSDN says about
     * returning the previous LPARAM value */
    lr = SendMessageA(hwnd, EM_SETIMESTATUS, EMSIS_COMPOSITIONSTRING, EIMES_GETCOMPSTRATONCE);
    ok(lr == 1, "Got unexpected lr %#Ix.\n", lr);
    lr = SendMessageA(hwnd, EM_GETIMESTATUS, EMSIS_COMPOSITIONSTRING, 0);
    ok(lr == EIMES_GETCOMPSTRATONCE, "Got unexpected lr %#Ix.\n", lr);

    lr = SendMessageA(hwnd, EM_SETIMESTATUS, EMSIS_COMPOSITIONSTRING, EIMES_CANCELCOMPSTRINFOCUS);
    ok(lr == 1, "Got unexpected lr %#Ix.\n", lr);
    lr = SendMessageA(hwnd, EM_GETIMESTATUS, EMSIS_COMPOSITIONSTRING, 0);
    ok(lr == EIMES_CANCELCOMPSTRINFOCUS, "Got unexpected lr %#Ix.\n", lr);

    lr = SendMessageA(hwnd, EM_SETIMESTATUS, EMSIS_COMPOSITIONSTRING, EIMES_COMPLETECOMPSTRKILLFOCUS);
    ok(lr == 1, "Got unexpected lr %#Ix.\n", lr);
    lr = SendMessageA(hwnd, EM_GETIMESTATUS, EMSIS_COMPOSITIONSTRING, 0);
    ok(lr == EIMES_COMPLETECOMPSTRKILLFOCUS, "Got unexpected lr %#Ix.\n", lr);

    lr = SendMessageA(hwnd, EM_SETIMESTATUS, EMSIS_COMPOSITIONSTRING, EIMES_GETCOMPSTRATONCE
                      | EIMES_CANCELCOMPSTRINFOCUS | EIMES_COMPLETECOMPSTRKILLFOCUS);
    ok(lr == 1, "Got unexpected lr %#Ix.\n", lr);
    lr = SendMessageA(hwnd, EM_GETIMESTATUS, EMSIS_COMPOSITIONSTRING, 0);
    ok(lr == (EIMES_GETCOMPSTRATONCE | EIMES_CANCELCOMPSTRINFOCUS | EIMES_COMPLETECOMPSTRKILLFOCUS),
       "Got unexpected lr %#Ix.\n", lr);

    lr = SendMessageA(hwnd, EM_SETIMESTATUS, EMSIS_COMPOSITIONSTRING, 0);
    ok(lr == 1, "Got unexpected lr %#Ix.\n", lr);
    lr = SendMessageA(hwnd, EM_GETIMESTATUS, EMSIS_COMPOSITIONSTRING, 0);
    ok(lr == 0, "Got unexpected lr %#Ix.\n", lr);

    /* Invalid EM_{GET|SET}IMESTATUS status types and flags */
    lr = SendMessageA(hwnd, EM_GETIMESTATUS, 0, 0);
    ok(lr == 1, "Got unexpected lr %#Ix.\n", lr);

    lr = SendMessageA(hwnd, EM_GETIMESTATUS, EMSIS_COMPOSITIONSTRING + 1, 0);
    ok(lr == 1, "Got unexpected lr %#Ix.\n", lr);

    lr = SendMessageA(hwnd, EM_SETIMESTATUS, 0, EIMES_GETCOMPSTRATONCE);
    ok(lr == 1, "Got unexpected lr %#Ix.\n", lr);
    lr = SendMessageA(hwnd, EM_GETIMESTATUS, EMSIS_COMPOSITIONSTRING, 0);
    ok(lr == 0, "Got unexpected lr %#Ix.\n", lr);

    lr = SendMessageA(hwnd, EM_SETIMESTATUS, EMSIS_COMPOSITIONSTRING + 1, EIMES_GETCOMPSTRATONCE);
    ok(lr == 1, "Got unexpected lr %#Ix.\n", lr);
    lr = SendMessageA(hwnd, EM_GETIMESTATUS, EMSIS_COMPOSITIONSTRING, 0);
    ok(lr == 0, "Got unexpected lr %#Ix.\n", lr);

    lr = SendMessageA(hwnd, EM_SETIMESTATUS, EMSIS_COMPOSITIONSTRING, 0xFFFFFFFF);
    ok(lr == 1, "Got unexpected lr %#Ix.\n", lr);
    lr = SendMessageA(hwnd, EM_GETIMESTATUS, EMSIS_COMPOSITIONSTRING, 0);
    ok(lr == 0xFFFF, "Got unexpected lr %#Ix.\n", lr);

    lr = SendMessageA(hwnd, EM_SETIMESTATUS, EMSIS_COMPOSITIONSTRING, 0);
    ok(lr == 1, "Got unexpected lr %#Ix.\n", lr);
    lr = SendMessageA(hwnd, EM_GETIMESTATUS, EMSIS_COMPOSITIONSTRING, 0);
    ok(lr == 0, "Got unexpected lr %#Ix.\n", lr);

    /* Test IME messages when EIMES_GETCOMPSTRATONCE is not set */
    old_proc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)edit_ime_subclass_proc);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)old_proc);
    flush_events();

    himc = ImmGetContext(hwnd);
    ret = ImmSetCompositionStringA(himc, SCS_SETSTR, "Wine", 4, NULL, 0);
    ok(ret, "ImmSetCompositionStringA failed.\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
    ok(ret, "ImmNotifyIME failed.\n");
    /* Note that the following message loop is necessary to get the WM_CHAR messages because they
     * are posted. Same for the later message loops in this function. */
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    if (hkl == korean_hkl)
        ok_sequence(sequences, COMBINED_SEQ_INDEX, wm_ime_composition_korean_seq,
                    "korean WM_IME_COMPOSITION", TRUE);
    else
        ok_sequence(sequences, COMBINED_SEQ_INDEX, wm_ime_composition_seq, "WM_IME_COMPOSITION", TRUE);

    /* Test that WM_IME_CHAR is passed to DefWindowProc() to get WM_CHAR */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    SendMessageA(hwnd, WM_IME_CHAR, '0', 1);
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok_sequence(sequences, COMBINED_SEQ_INDEX, wm_ime_char_seq, "WM_IME_CHAR", FALSE);

    /* Test IME messages when EIMES_GETCOMPSTRATONCE is set */
    lr = SendMessageA(hwnd, EM_SETIMESTATUS, EMSIS_COMPOSITIONSTRING, EIMES_GETCOMPSTRATONCE);
    ok(lr == 1, "Got unexpected lr %#Ix.\n", lr);
    lr = SendMessageA(hwnd, EM_GETIMESTATUS, EMSIS_COMPOSITIONSTRING, 0);
    ok(lr == EIMES_GETCOMPSTRATONCE, "Got unexpected lr %#Ix.\n", lr);

    ret = ImmSetCompositionStringA(himc, SCS_SETSTR, "Wine", 4, NULL, 0);
    ok(ret, "ImmSetCompositionStringA failed.\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
    ok(ret, "ImmNotifyIME failed.\n");
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    if (hkl == korean_hkl)
        ok_sequence(sequences, COMBINED_SEQ_INDEX, wm_ime_composition_korean_seq,
                    "Korean WM_IME_COMPOSITION with EIMES_GETCOMPSTRATONCE", TRUE);
    else
        ok_sequence(sequences, COMBINED_SEQ_INDEX, eimes_getcompstratonce_seq,
                    "WM_IME_COMPOSITION with EIMES_GETCOMPSTRATONCE", TRUE);

    /* Test that WM_IME_CHAR is passed to DefWindowProc() to get WM_CHAR with EIMES_GETCOMPSTRATONCE */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    SendMessageA(hwnd, WM_IME_CHAR, '0', 1);
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok_sequence(sequences, COMBINED_SEQ_INDEX, wm_ime_char_seq, "WM_IME_CHAR", FALSE);

    ImmReleaseContext(hwnd, himc);
    DestroyWindow(hwnd);
}

START_TEST(edit)
{
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;
    BOOL b;

    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    hinst = GetModuleHandleA(NULL);
    b = register_classes();
    ok(b, "Failed to register test classes.\n");
    if (!b) return;

    test_edit_control_1();
    test_edit_control_2();
    test_edit_control_3();
    test_char_from_pos();
    test_edit_control_5();
    test_edit_control_6();
    test_edit_control_limittext();
    test_edit_control_scroll();
    test_initialization();
    test_margins();
    test_margins_font_change();
    test_text_position();
    test_espassword();
    test_undo();
    test_enter();
    test_tab();
    test_edit_dialog();
    test_multi_edit_dialog();
    test_wantreturn_edit_dialog();
    test_singleline_wantreturn_edit_dialog();
    test_child_edit_wmkeydown();
    test_fontsize();
    test_dialogmode();
    test_contextmenu();
    test_EM_GETHANDLE();
    test_paste();
    test_EM_GETLINE();
    test_wordbreak_proc();
    test_change_focus();
    test_cue_banner();
    test_ime();

    UnregisterWindowClasses();

    unload_v6_module(ctx_cookie, hCtx);
}
