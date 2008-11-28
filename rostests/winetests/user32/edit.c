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

#include <assert.h>
#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"

#ifndef ES_COMBO
#define ES_COMBO 0x200
#endif

#define ID_EDITTEST2 99
#define MAXLEN 200

struct edit_notify {
    int en_change, en_maxtext, en_update;
};

static struct edit_notify notifications;

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
                    PostMessage(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 1:
                    PostMessage(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 2:
                    PostMessage(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessage(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;

                /* test cases for pressing enter */
                case 3:
                    num_ok_commands = 0;
                    PostMessage(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 1);
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
                    PostMessage(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    break;
                case 1:
                    PostMessage(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    break;
                case 2:
                    PostMessage(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                /* more test cases for WM_CHAR */
                case 3:
                    PostMessage(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 4:
                    PostMessage(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 5:
                    PostMessage(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;

                /* more test cases for WM_KEYDOWN + WM_CHAR */
                case 6:
                    PostMessage(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    PostMessage(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 7:
                    PostMessage(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessage(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;
                case 8:
                    PostMessage(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessage(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                /* multiple tab tests */
                case 9:
                    PostMessage(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessage(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 2);
                    break;
                case 10:
                    PostMessage(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessage(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessage(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 2);
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
                    len = SendMessage(hedit, WM_GETTEXTLENGTH, 0, 0);
                    if (len == 0)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                case 1:
                    len = SendMessage(hedit, WM_GETTEXTLENGTH, 0, 0);
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
                    PostMessage(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    break;
                case 1:
                    PostMessage(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    break;
                case 2:
                    PostMessage(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                /* test cases for WM_CHAR */
                case 3:
                    PostMessage(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 4:
                    PostMessage(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 5:
                    PostMessage(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;

                /* test cases for WM_KEYDOWN + WM_CHAR */
                case 6:
                    PostMessage(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    PostMessage(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    break;
                case 7:
                    PostMessage(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessage(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    break;
                case 8:
                    PostMessage(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessage(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 1);
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
            int len = SendMessage(hedit, WM_GETTEXTLENGTH, 0, 0);

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
                    PostMessage(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    break;
                case 1:
                    PostMessage(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 2:
                    PostMessage(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                /* test cases for WM_CHAR */
                case 3:
                    PostMessage(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 4:
                    PostMessage(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 2);
                    break;
                case 5:
                    PostMessage(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;

                /* test cases for WM_KEYDOWN + WM_CHAR */
                case 6:
                    PostMessage(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    PostMessage(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 7:
                    PostMessage(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessage(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 2);
                    break;
                case 8:
                    PostMessage(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessage(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessage(hdlg, WM_USER, 0xdeadbeef, 1);
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
            int len = SendMessage(hedit, WM_GETTEXTLENGTH, 0, 0);

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
static const char szEditTextPositionClass[] = "EditTextPositionWindowClass";

static HWND create_editcontrol (DWORD style, DWORD exstyle)
{
    HWND handle;

    handle = CreateWindowEx(exstyle,
			  "EDIT",
			  "Test Text",
			  style,
			  10, 10, 300, 300,
			  NULL, NULL, hinst, NULL);
    assert (handle);
    if (winetest_interactive)
	ShowWindow (handle, SW_SHOW);
    return handle;
}

static HWND create_child_editcontrol (DWORD style, DWORD exstyle)
{
    HWND parentWnd;
    HWND editWnd;
    RECT rect;
    
    rect.left = 0;
    rect.top = 0;
    rect.right = 300;
    rect.bottom = 300;
    assert(AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE));
    
    parentWnd = CreateWindowEx(0,
                            szEditTextPositionClass,
                            "Edit Test",
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            rect.right - rect.left, rect.bottom - rect.top,
                            NULL, NULL, hinst, NULL);
    assert(parentWnd);

    editWnd = CreateWindowEx(exstyle,
                            "EDIT",
                            "Test Text",
                            WS_CHILD | style,
                            0, 0, 300, 300,
                            parentWnd, NULL, hinst, NULL);
    assert(editWnd);
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
        "The client height should be %ld, but is %ld\n",
        (long)Height, (long)(ClientRect.bottom - ClientRect.top));
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
    ok(r == (ES_AUTOVSCROLL | ES_AUTOHSCROLL), "Wrong style expected 0xc0 got: 0x%x\n", r);
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessage(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS),
            "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS got %x\n", r);
    }
    DestroyWindow (hwEdit);

    trace("EDIT: Single line want returns\n");
    hwEdit = create_editcontrol(ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN), "Wrong style expected 0x10c0 got: 0x%x\n", r);
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessage(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS),
            "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS got %x\n", r);
    }
    DestroyWindow (hwEdit);

    trace("EDIT: Multiline line\n");
    hwEdit = create_editcontrol(ES_MULTILINE | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE), "Wrong style expected 0xc4 got: 0x%x\n", r);
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessage(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS),
            "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS got %x\n", r);
    }
    DestroyWindow (hwEdit);

    trace("EDIT: Multi line want returns\n");
    hwEdit = create_editcontrol(ES_MULTILINE | WS_VSCROLL | ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE), "Wrong style expected 0x10c4 got: 0x%x\n", r);
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessage(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS),
            "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS got %x\n", r);
    }
    DestroyWindow (hwEdit);
}

/* WM_SETTEXT is implemented by selecting all text, and then replacing the
 * selection.  This test checks that the first 'select all' doesn't generate
 * an UPDATE message which can escape and (via a handler) change the
 * selection, which would cause WM_SETTEXT to break.  This old bug
 * was fixed 18-Mar-2005; we check here to ensure it doesn't regress.
 */
static void test_edit_control_2(void)
{
    HWND hwndMain;
    char szLocalString[MAXLEN];
    LONG r;

    /* Create main and edit windows. */
    hwndMain = CreateWindow(szEditTest2Class, "ET2", WS_OVERLAPPEDWINDOW,
                            0, 0, 200, 200, NULL, NULL, hinst, NULL);
    assert(hwndMain);
    if (winetest_interactive)
        ShowWindow (hwndMain, SW_SHOW);

    hwndET2 = CreateWindow("EDIT", NULL,
                           WS_CHILD|WS_BORDER|ES_LEFT|ES_AUTOHSCROLL,
                           0, 0, 150, 50, /* important this not be 0 size. */
                           hwndMain, (HMENU) ID_EDITTEST2, hinst, NULL);
    assert(hwndET2);
    if (winetest_interactive)
        ShowWindow (hwndET2, SW_SHOW);

    trace("EDIT: SETTEXT atomicity\n");
    /* Send messages to "type" in the word 'foo'. */
    r = SendMessage(hwndET2, WM_CHAR, 'f', 1);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);
    r = SendMessage(hwndET2, WM_CHAR, 'o', 1);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);
    r = SendMessage(hwndET2, WM_CHAR, 'o', 1);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);
    /* 'foo' should have been changed to 'bar' by the UPDATE handler. */
    GetWindowText(hwndET2, szLocalString, MAXLEN);
    ok(lstrcmp(szLocalString, "bar")==0,
       "Wrong contents of edit: %s\n", szLocalString);

    /* OK, done! */
    DestroyWindow (hwndET2);
    DestroyWindow (hwndMain);
}

static void ET2_check_change(void) {
   char szLocalString[MAXLEN];
   /* This EN_UPDATE handler changes any 'foo' to 'bar'. */
   GetWindowText(hwndET2, szLocalString, MAXLEN);
   if (lstrcmp(szLocalString, "foo")==0) {
       lstrcpy(szLocalString, "bar");
       SendMessage(hwndET2, WM_SETTEXT, 0, (LPARAM) szLocalString);
   }
   /* always leave the cursor at the end. */
   SendMessage(hwndET2, EM_SETSEL, MAXLEN - 1, MAXLEN - 1);
}
static void ET2_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (id==ID_EDITTEST2 && codeNotify == EN_UPDATE)
        ET2_check_change();
}
static LRESULT CALLBACK ET2_WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg) {
    case WM_COMMAND:
        ET2_OnCommand(hwnd, LOWORD(wParam), (HWND)lParam, HIWORD(wParam));
        break;
    }
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

static void zero_notify(void)
{
    notifications.en_change = 0;
    notifications.en_maxtext = 0;
    notifications.en_update = 0;
}

#define test_notify(enchange, enmaxtext, enupdate) \
    ok(notifications.en_change == enchange, "expected %d EN_CHANGE notifications, " \
    "got %d\n", enchange, notifications.en_change); \
    ok(notifications.en_maxtext == enmaxtext, "expected %d EN_MAXTEXT notifications, " \
    "got %d\n", enmaxtext, notifications.en_maxtext); \
    ok(notifications.en_update == enupdate, "expected %d EN_UPDATE notifications, " \
    "got %d\n", enupdate, notifications.en_update)


static LRESULT CALLBACK edit3_wnd_procA(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_COMMAND:
            switch (HIWORD(wParam)) {
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

/* Test behaviour of WM_SETTEXT, WM_REPLACESEL and notificatisons sent in response
 * to these messages.
 */
static void test_edit_control_3(void)
{
    HWND hWnd;
    HWND hParent;
    int len;
    static const char *str = "this is a long string.";
    static const char *str2 = "this is a long string.\r\nthis is a long string.\r\nthis is a long string.\r\nthis is a long string.";

    trace("EDIT: Test notifications\n");

    hParent = CreateWindowExA(0,
              szEditTest3Class,
              NULL,
              0,
              CW_USEDEFAULT, CW_USEDEFAULT, 10, 10,
              NULL, NULL, NULL, NULL);
    assert(hParent);

    trace("EDIT: Single line, no ES_AUTOHSCROLL\n");
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              0,
              10, 10, 50, 50,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) > len, "text should have been truncated\n");
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
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              ES_AUTOHSCROLL,
              10, 10, 50, 50,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

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
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              ES_MULTILINE,
              10, 10, 50, 50,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
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
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              ES_MULTILINE | ES_AUTOHSCROLL,
              10, 10, 50, 50,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

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
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
              10, 10, 50, 50,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

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

/* Test EM_CHARFROMPOS and EM_POSFROMCHAR
 */
static void test_edit_control_4(void)
{
    HWND hwEdit;
    int lo, hi, mid;
    int ret;
    int i;

    trace("EDIT: Test EM_CHARFROMPOS and EM_POSFROMCHAR\n");
    hwEdit = create_editcontrol(ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessage(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessage(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_RIGHT | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessage(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessage(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_CENTER | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessage(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessage(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessage(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2 +1;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok((0 == ret || 1 == ret /* Vista */), "expected 0 or 1 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessage(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_MULTILINE | ES_RIGHT | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessage(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2 +1;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok((0 == ret || 1 == ret /* Vista */), "expected 0 or 1 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessage(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_MULTILINE | ES_CENTER | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessage(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2 +1;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok((0 == ret || 1 == ret /* Vista */), "expected 0 or 1 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessage(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);
}

/* Test if creating edit control without ES_AUTOHSCROLL and ES_AUTOVSCROLL
 * truncates text that doesn't fit.
 */
static void test_edit_control_5(void)
{
    static const char *str = "test\r\ntest";
    HWND hWnd;
    int len;

    hWnd = CreateWindowEx(0,
              "EDIT",
              str,
              0,
              10, 10, 1, 1,
              NULL, NULL, NULL, NULL);
    assert(hWnd);

    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    DestroyWindow(hWnd);

    hWnd = CreateWindowEx(0,
              "EDIT",
              str,
              ES_MULTILINE,
              10, 10, 1, 1,
              NULL, NULL, NULL, NULL);
    assert(hWnd);

    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    DestroyWindow(hWnd);
}

static void test_edit_control_limittext(void)
{
    HWND hwEdit;
    DWORD r;

    /* Test default limit for single-line control */
    trace("EDIT: buffer limit for single-line\n");
    hwEdit = create_editcontrol(ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = SendMessage(hwEdit, EM_GETLIMITTEXT, 0, 0);
    ok(r == 30000, "Incorrect default text limit, expected 30000 got %u\n", r);
    SendMessage(hwEdit, EM_SETLIMITTEXT, 0, 0);
    r = SendMessage(hwEdit, EM_GETLIMITTEXT, 0, 0);
    /* Win9x+ME: 32766; WinNT: 2147483646UL */
    ok( (r == 32766) || (r == 2147483646UL),
        "got limit %u (expected 32766 or 2147483646)\n", r);
    DestroyWindow(hwEdit);

    /* Test default limit for multi-line control */
    trace("EDIT: buffer limit for multi-line\n");
    hwEdit = create_editcontrol(ES_MULTILINE | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = SendMessage(hwEdit, EM_GETLIMITTEXT, 0, 0);
    ok(r == 30000, "Incorrect default text limit, expected 30000 got %u\n", r);
    SendMessage(hwEdit, EM_SETLIMITTEXT, 0, 0);
    r = SendMessage(hwEdit, EM_GETLIMITTEXT, 0, 0);
    /* Win9x+ME: 65535; WinNT: 4294967295UL */
    ok( (r == 65535) || (r == 4294967295UL),
        "got limit %u (expected 65535 or 4294967295)\n", r);
    DestroyWindow(hwEdit);
}

static void test_margins(void)
{
    HWND hwEdit;
    RECT old_rect, new_rect;
    INT old_left_margin, old_right_margin;
    DWORD old_margins, new_margins;

    hwEdit = create_editcontrol(WS_BORDER | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    
    old_margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    old_left_margin = LOWORD(old_margins);
    old_right_margin = HIWORD(old_margins);
    
    /* Check if setting the margins works */
    
    SendMessage(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN, MAKELONG(10, 0));
    new_margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(new_margins) == 10, "Wrong left margin: %d\n", LOWORD(new_margins));
    ok(HIWORD(new_margins) == old_right_margin, "Wrong right margin: %d\n", HIWORD(new_margins));
    
    SendMessage(hwEdit, EM_SETMARGINS, EC_RIGHTMARGIN, MAKELONG(0, 10));
    new_margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(new_margins) == 10, "Wrong left margin: %d\n", LOWORD(new_margins));
    ok(HIWORD(new_margins) == 10, "Wrong right margin: %d\n", HIWORD(new_margins));
    
    
    /* The size of the rectangle must decrease if we increase the margin */
    
    SendMessage(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(5, 5));
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM)&old_rect);
    SendMessage(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(15, 20));
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM)&new_rect);
    ok(new_rect.left == old_rect.left + 10, "The left border of the rectangle is wrong\n");
    ok(new_rect.right == old_rect.right - 15, "The right border of the rectangle is wrong\n");
    ok(new_rect.top == old_rect.top, "The top border of the rectangle must not change\n");
    ok(new_rect.bottom == old_rect.bottom, "The bottom border of the rectangle must not change\n");
    
    
    /* If we set the margin to same value as the current margin,
       the rectangle must not change */
    
    SendMessage(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(10, 10));
    old_rect.left = 1;
    old_rect.right = 99;
    old_rect.top = 1;
    old_rect.bottom = 99;
    SendMessage(hwEdit, EM_SETRECT, 0, (LPARAM)&old_rect);    
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM)&old_rect);
    SendMessage(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(10, 10));
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM)&new_rect);
    ok(new_rect.left == old_rect.left, "The left border of the rectangle has changed\n");
    ok(new_rect.right == old_rect.right, "The right border of the rectangle has changed\n");
    ok(new_rect.top == old_rect.top, "The top border of the rectangle has changed\n");
    ok(new_rect.bottom == old_rect.bottom, "The bottom border of the rectangle has changed\n");

    DestroyWindow (hwEdit);
}

static INT CALLBACK find_font_proc(const LOGFONT *elf, const TEXTMETRIC *ntm, DWORD type, LPARAM lParam)
{
    return 0;
}

static void test_margins_font_change(void)
{
    HWND hwEdit;
    DWORD margins, font_margins;
    LOGFONT lf;
    HFONT hfont, hfont2;
    HDC hdc = GetDC(0);

    if(EnumFontFamiliesA(hdc, "Arial", find_font_proc, 0))
    {
        trace("Arial not found - skipping font change margin tests\n");
        ReleaseDC(0, hdc);
        return;
    }
    ReleaseDC(0, hdc);

    hwEdit = create_child_editcontrol(0, 0);

    SetWindowPos(hwEdit, NULL, 10, 10, 1000, 100, SWP_NOZORDER | SWP_NOACTIVATE);

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Arial");
    lf.lfHeight = 16;
    lf.lfCharSet = DEFAULT_CHARSET;
    hfont = CreateFontIndirectA(&lf);
    lf.lfHeight = 30;
    hfont2 = CreateFontIndirectA(&lf);

    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    font_margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(font_margins) != 0, "got %d\n", LOWORD(font_margins));
    ok(HIWORD(font_margins) != 0, "got %d\n", HIWORD(font_margins));

    /* With 'small' edit controls, test that the margin doesn't get set */
    SetWindowPos(hwEdit, NULL, 10, 10, 16, 100, SWP_NOZORDER | SWP_NOACTIVATE);
    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0,0));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 0 || broken(LOWORD(margins) == LOWORD(font_margins)), /* win95 */
       "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 0 || broken(HIWORD(margins) == HIWORD(font_margins)), /* win95 */
       "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,0));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 1 || broken(LOWORD(margins) == LOWORD(font_margins)), /* win95 */
       "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 0 || broken(HIWORD(margins) == HIWORD(font_margins)), /* win95 */
       "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,1));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 1 || broken(LOWORD(margins) == LOWORD(font_margins)), /* win95 */
       "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 1 || broken(HIWORD(margins) == HIWORD(font_margins)), /* win95 */
       "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(EC_USEFONTINFO,EC_USEFONTINFO));
    margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 1 || broken(LOWORD(margins) == LOWORD(font_margins)), /* win95 */
       "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 1 || broken(HIWORD(margins) == HIWORD(font_margins)), /* win95 */
       "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont2, 0);
    margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 1 || broken(LOWORD(margins) != 1 && LOWORD(margins) != LOWORD(font_margins)), /* win95 */
       "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 1 || broken(HIWORD(margins) != 1 && HIWORD(margins) != HIWORD(font_margins)), /* win95 */
       "got %d\n", HIWORD(margins));

    /* Above a certain size threshold then the margin is updated */
    SetWindowPos(hwEdit, NULL, 10, 10, 1000, 100, SWP_NOZORDER | SWP_NOACTIVATE);
    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,0));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == LOWORD(font_margins), "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == HIWORD(font_margins), "got %d\n", HIWORD(margins)); 

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,1));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == LOWORD(font_margins), "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == HIWORD(font_margins), "got %d\n", HIWORD(margins)); 

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(EC_USEFONTINFO,EC_USEFONTINFO));
    margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == LOWORD(font_margins), "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == HIWORD(font_margins), "got %d\n", HIWORD(margins)); 
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont2, 0);
    margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) != LOWORD(font_margins) || broken(LOWORD(margins) == LOWORD(font_margins)), /* win98 */
       "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) != HIWORD(font_margins), "got %d\n", HIWORD(margins)); 

    SendMessageA(hwEdit, WM_SETFONT, 0, 0);
    
    DeleteObject(hfont2);
    DeleteObject(hfont);
    destroy_child_editcontrol(hwEdit);

}

#define edit_pos_ok(exp, got, txt) \
    ok(exp == got, "wrong " #txt " expected %d got %d\n", exp, got);

#define check_pos(hwEdit, set_height, test_top, test_height, test_left) \
do { \
    RECT format_rect; \
    int left_margin; \
    set_client_height(hwEdit, set_height); \
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &format_rect); \
    left_margin = LOWORD(SendMessage(hwEdit, EM_GETMARGINS, 0, 0)); \
    edit_pos_ok(test_top, format_rect.top, vertical position); \
    edit_pos_ok((int)test_height, format_rect.bottom - format_rect.top, height); \
    edit_pos_ok(test_left, format_rect.left - left_margin, left); \
} while(0)

static void test_text_position_style(DWORD style)
{
    HWND hwEdit;
    HFONT font, oldFont;
    HDC dc;
    TEXTMETRIC metrics;
    INT b, bm, b2, b3;
    BOOL single_line = !(style & ES_MULTILINE);

    b = GetSystemMetrics(SM_CYBORDER) + 1;
    b2 = 2 * b;
    b3 = 3 * b;
    bm = b2 - 1;
    
    /* Get a stock font for which we can determine the metrics */
    assert(font = GetStockObject(SYSTEM_FONT));
    assert(dc = GetDC(NULL));
    oldFont = SelectObject(dc, font);
    assert(GetTextMetrics(dc, &metrics));    
    SelectObject(dc, oldFont);
    ReleaseDC(NULL, dc);
    
    /* Windows' edit control has some bugs in multi-line mode:
     * - Sometimes the format rectangle doesn't get updated
     *   (see workaround in set_client_height())
     * - If the height of the control is smaller than the height of a text
     *   line, the format rectangle is still as high as a text line
     *   (higher than the client rectangle) and the caret is not shown
     */
    
    /* Edit controls that are in a parent window */
       
    hwEdit = create_child_editcontrol(style | WS_VISIBLE, 0);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) font, (LPARAM) FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 0);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , 0);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , 0);
    check_pos(hwEdit, metrics.tmHeight +  2, 0, metrics.tmHeight    , 0);
    check_pos(hwEdit, metrics.tmHeight + 10, 0, metrics.tmHeight    , 0);
    destroy_child_editcontrol(hwEdit);

    hwEdit = create_child_editcontrol(style | WS_BORDER | WS_VISIBLE, 0);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) font, (LPARAM) FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, b);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight + bm, 0, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight + b2, b, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight + b3, b, metrics.tmHeight    , b);
    destroy_child_editcontrol(hwEdit);

    hwEdit = create_child_editcontrol(style | WS_VISIBLE, WS_EX_CLIENTEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) font, (LPARAM) FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 1);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  2, 1, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight + 10, 1, metrics.tmHeight    , 1);
    destroy_child_editcontrol(hwEdit);

    hwEdit = create_child_editcontrol(style | WS_BORDER | WS_VISIBLE, WS_EX_CLIENTEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) font, (LPARAM) FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 1);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  2, 1, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight + 10, 1, metrics.tmHeight    , 1);
    destroy_child_editcontrol(hwEdit);


    /* Edit controls that are popup windows */
    
    hwEdit = create_editcontrol(style | WS_POPUP, 0);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) font, (LPARAM) FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 0);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , 0);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , 0);
    check_pos(hwEdit, metrics.tmHeight +  2, 0, metrics.tmHeight    , 0);
    check_pos(hwEdit, metrics.tmHeight + 10, 0, metrics.tmHeight    , 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(style | WS_POPUP | WS_BORDER, 0);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) font, (LPARAM) FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, b);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight + bm, 0, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight + b2, b, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight + b3, b, metrics.tmHeight    , b);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(style | WS_POPUP, WS_EX_CLIENTEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) font, (LPARAM) FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 1);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  2, 1, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight + 10, 1, metrics.tmHeight    , 1);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(style | WS_POPUP | WS_BORDER, WS_EX_CLIENTEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) font, (LPARAM) FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 1);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  2, 1, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight + 10, 1, metrics.tmHeight    , 1);
    DestroyWindow(hwEdit);
}

static void test_text_position(void)
{
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
    ok(r == ES_PASSWORD, "Wrong style expected 0x%x got: 0x%x\n", ES_PASSWORD, r);
    /* set text */
    r = SendMessage(hwEdit , WM_SETTEXT, 0, (LPARAM) password);
    ok(r == TRUE, "Expected: %d, got: %d\n", TRUE, r);

    /* select all, cut (ctrl-x) */
    SendMessage(hwEdit, EM_SETSEL, 0, -1);
    r = SendMessage(hwEdit, WM_CHAR, 24, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    r = SendMessage(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(r == strlen(password), "Expected: %s, got len %d\n", password, r);
    ok(strcmp(buffer, password) == 0, "expected %s, got %s\n", password, buffer);

    r = OpenClipboard(hwEdit);
    ok(r == TRUE, "expected %d, got %d\n", TRUE, r);
    r = EmptyClipboard();
    ok(r == TRUE, "expected %d, got %d\n", TRUE, r);
    r = CloseClipboard();
    ok(r == TRUE, "expected %d, got %d\n", TRUE, r);

    /* select all, copy (ctrl-c) and paste (ctrl-v) */
    SendMessage(hwEdit, EM_SETSEL, 0, -1);
    r = SendMessage(hwEdit, WM_CHAR, 3, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);
    r = SendMessage(hwEdit, WM_CHAR, 22, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessage(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(r == 0, "Expected: 0, got: %d\n", r);
    ok(strcmp(buffer, "") == 0, "expected empty string, got %s\n", buffer);

    DestroyWindow (hwEdit);
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
    ok(0 == r, "Wrong style expected 0x%x got: 0x%x\n", 0, r);

    /* set text */
    r = SendMessage(hwEdit , WM_SETTEXT, 0, (LPARAM) text);
    ok(TRUE == r, "Expected: %d, got: %d\n", TRUE, r);

    /* select all, */
    cpMin = cpMax = 0xdeadbeef;
    SendMessage(hwEdit, EM_SETSEL, 0, -1);
    r = SendMessage(hwEdit, EM_GETSEL, (WPARAM) &cpMin, (LPARAM) &cpMax);
    ok((strlen(text) << 16) == r, "Unexpected length %d\n", r);
    ok(0 == cpMin, "Expected: %d, got %d\n", 0, cpMin);
    ok(9 == cpMax, "Expected: %d, got %d\n", 9, cpMax);

    /* cut (ctrl-x) */
    r = SendMessage(hwEdit, WM_CHAR, 24, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessage(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(0 == r, "Expected: %d, got len %d\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected %s, got %s\n", "", buffer);

    /* undo (ctrl-z) */
    r = SendMessage(hwEdit, WM_CHAR, 26, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessage(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(strlen(text) == r, "Unexpected length %d\n", r);
    ok(0 == strcmp(buffer, text), "expected %s, got %s\n", text, buffer);

    /* undo again (ctrl-z) */
    r = SendMessage(hwEdit, WM_CHAR, 26, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessage(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(r == 0, "Expected: %d, got len %d\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected %s, got %s\n", "", buffer);

    DestroyWindow (hwEdit);
}

static void test_enter(void)
{
    HWND hwEdit;
    LONG r;
    char buffer[16];

    /* multiline */
    hwEdit = create_editcontrol(ES_MULTILINE, 0);
    r = get_edit_style(hwEdit);
    ok(ES_MULTILINE == r, "Wrong style expected 0x%x got: 0x%x\n", ES_MULTILINE, r);

    /* set text */
    r = SendMessage(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %d\n", TRUE, r);

    r = SendMessage(hwEdit, WM_CHAR, VK_RETURN, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessage(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(2 == r, "Expected: %d, got len %d\n", 2, r);
    ok(0 == strcmp(buffer, "\r\n"), "expected \"\\r\\n\", got \"%s\"\n", buffer);

    DestroyWindow (hwEdit);

    /* single line */
    hwEdit = create_editcontrol(0, 0);
    r = get_edit_style(hwEdit);
    ok(0 == r, "Wrong style expected 0x%x got: 0x%x\n", 0, r);

    /* set text */
    r = SendMessage(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %d\n", TRUE, r);

    r = SendMessage(hwEdit, WM_CHAR, VK_RETURN, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessage(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(0 == r, "Expected: %d, got len %d\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected \"\", got \"%s\"\n", buffer);

    DestroyWindow (hwEdit);

    /* single line with ES_WANTRETURN */
    hwEdit = create_editcontrol(ES_WANTRETURN, 0);
    r = get_edit_style(hwEdit);
    ok(ES_WANTRETURN == r, "Wrong style expected 0x%x got: 0x%x\n", ES_WANTRETURN, r);

    /* set text */
    r = SendMessage(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %d\n", TRUE, r);

    r = SendMessage(hwEdit, WM_CHAR, VK_RETURN, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessage(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(0 == r, "Expected: %d, got len %d\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected \"\", got \"%s\"\n", buffer);

    DestroyWindow (hwEdit);
}

static void test_tab(void)
{
    HWND hwEdit;
    LONG r;
    char buffer[16];

    /* multiline */
    hwEdit = create_editcontrol(ES_MULTILINE, 0);
    r = get_edit_style(hwEdit);
    ok(ES_MULTILINE == r, "Wrong style expected 0x%x got: 0x%x\n", ES_MULTILINE, r);

    /* set text */
    r = SendMessage(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %d\n", TRUE, r);

    r = SendMessage(hwEdit, WM_CHAR, VK_TAB, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessage(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(1 == r, "Expected: %d, got len %d\n", 1, r);
    ok(0 == strcmp(buffer, "\t"), "expected \"\\t\", got \"%s\"\n", buffer);

    DestroyWindow (hwEdit);

    /* single line */
    hwEdit = create_editcontrol(0, 0);
    r = get_edit_style(hwEdit);
    ok(0 == r, "Wrong style expected 0x%x got: 0x%x\n", 0, r);

    /* set text */
    r = SendMessage(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %d\n", TRUE, r);

    r = SendMessage(hwEdit, WM_CHAR, VK_TAB, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessage(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(0 == r, "Expected: %d, got len %d\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected \"\", got \"%s\"\n", buffer);

    DestroyWindow (hwEdit);
}

static void test_edit_dialog(void)
{
    int r;

    /* from bug 11841 */
    r = DialogBoxParam(hinst, "EDIT_READONLY_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 0);
    ok(333 == r, "Expected %d, got %d\n", 333, r);
    r = DialogBoxParam(hinst, "EDIT_READONLY_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 1);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParam(hinst, "EDIT_READONLY_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* more tests for WM_CHAR */
    r = DialogBoxParam(hinst, "EDIT_READONLY_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_READONLY_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_READONLY_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* more tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParam(hinst, "EDIT_READONLY_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 6);
    todo_wine ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_READONLY_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 7);
    todo_wine ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_READONLY_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 8);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests with an editable edit control */
    r = DialogBoxParam(hinst, "EDIT_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 0);
    ok(333 == r, "Expected %d, got %d\n", 333, r);
    r = DialogBoxParam(hinst, "EDIT_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 1);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParam(hinst, "EDIT_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_CHAR */
    r = DialogBoxParam(hinst, "EDIT_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParam(hinst, "EDIT_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 6);
    todo_wine ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 7);
    todo_wine ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 8);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* multiple tab tests */
    r = DialogBoxParam(hinst, "EDIT_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 9);
    ok(22 == r, "Expected %d, got %d\n", 22, r);
    r = DialogBoxParam(hinst, "EDIT_DIALOG", NULL, (DLGPROC)edit_dialog_proc, 10);
    ok(33 == r, "Expected %d, got %d\n", 33, r);
}

static void test_multi_edit_dialog(void)
{
    int r;

    /* test for multiple edit dialogs (bug 12319) */
    r = DialogBoxParam(hinst, "MULTI_EDIT_DIALOG", NULL, (DLGPROC)multi_edit_dialog_proc, 0);
    ok(2222 == r, "Expected %d, got %d\n", 2222, r);
    r = DialogBoxParam(hinst, "MULTI_EDIT_DIALOG", NULL, (DLGPROC)multi_edit_dialog_proc, 1);
    ok(1111 == r, "Expected %d, got %d\n", 1111, r);
    r = DialogBoxParam(hinst, "MULTI_EDIT_DIALOG", NULL, (DLGPROC)multi_edit_dialog_proc, 2);
    ok(2222 == r, "Expected %d, got %d\n", 2222, r);
    r = DialogBoxParam(hinst, "MULTI_EDIT_DIALOG", NULL, (DLGPROC)multi_edit_dialog_proc, 3);
    ok(11 == r, "Expected %d, got %d\n", 11, r);
}

static void test_wantreturn_edit_dialog(void)
{
    int r;

    /* tests for WM_KEYDOWN */
    r = DialogBoxParam(hinst, "EDIT_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_wantreturn_dialog_proc, 0);
    ok(333 == r, "Expected %d, got %d\n", 333, r);
    r = DialogBoxParam(hinst, "EDIT_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_wantreturn_dialog_proc, 1);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_wantreturn_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_CHAR */
    r = DialogBoxParam(hinst, "EDIT_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_wantreturn_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_wantreturn_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_wantreturn_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParam(hinst, "EDIT_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_wantreturn_dialog_proc, 6);
    todo_wine ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_wantreturn_dialog_proc, 7);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_wantreturn_dialog_proc, 8);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
}

static void test_singleline_wantreturn_edit_dialog(void)
{
    int r;

    /* tests for WM_KEYDOWN */
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 0);
    ok(222 == r, "Expected %d, got %d\n", 222, r);
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 1);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_CHAR */
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 6);
    ok(222 == r, "Expected %d, got %d\n", 222, r);
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 7);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 8);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN */
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 0);
    ok(222 == r, "Expected %d, got %d\n", 222, r);
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 1);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_CHAR */
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 6);
    ok(222 == r, "Expected %d, got %d\n", 222, r);
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 7);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParam(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, (DLGPROC)edit_singleline_dialog_proc, 8);
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
    SetWindowLongPtr(hwParent, GWLP_WNDPROC, (LONG_PTR)child_edit_wmkeydown_proc);
    r = SendMessage(hwEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    ok(0 == child_edit_wmkeydown_num_messages, "expected 0, got %d\n", child_edit_wmkeydown_num_messages);
    destroy_child_editcontrol(hwEdit);
}

static BOOL RegisterWindowClasses (void)
{
    WNDCLASSA test2;
    WNDCLASSA test3;
    WNDCLASSA text_position;
    
    test2.style = 0;
    test2.lpfnWndProc = ET2_WndProc;
    test2.cbClsExtra = 0;
    test2.cbWndExtra = 0;
    test2.hInstance = hinst;
    test2.hIcon = NULL;
    test2.hCursor = LoadCursorA (NULL, IDC_ARROW);
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
    test3.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    test3.hbrBackground = GetStockObject(WHITE_BRUSH);
    test3.lpszMenuName = NULL;
    test3.lpszClassName = szEditTest3Class;
    if (!RegisterClassA(&test3)) return FALSE;

    text_position.style = CS_HREDRAW | CS_VREDRAW;
    text_position.cbClsExtra = 0;
    text_position.cbWndExtra = 0;
    text_position.hInstance = hinst;
    text_position.hIcon = NULL;
    text_position.hCursor = LoadCursorA(NULL, IDC_ARROW);
    text_position.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    text_position.lpszMenuName = NULL;
    text_position.lpszClassName = szEditTextPositionClass;
    text_position.lpfnWndProc = DefWindowProc;
    if (!RegisterClassA(&text_position)) return FALSE;

    return TRUE;
}

static void UnregisterWindowClasses (void)
{
    UnregisterClassA(szEditTest2Class, hinst);
    UnregisterClassA(szEditTest3Class, hinst);
    UnregisterClassA(szEditTextPositionClass, hinst);
}

void test_fontsize(void)
{
    HWND hwEdit;
    HFONT hfont;
    LOGFONT lf;
    LONG r;
    char szLocalString[MAXLEN];

    memset(&lf,0,sizeof(LOGFONTA));
    strcpy(lf.lfFaceName,"Arial");
    lf.lfHeight = -300; /* taller than the edit box */
    lf.lfWeight = 500;
    hfont = CreateFontIndirect(&lf);

    trace("EDIT: Oversized font (Multi line)\n");
    hwEdit= CreateWindow("EDIT", NULL, ES_MULTILINE|ES_AUTOHSCROLL,
                           0, 0, 150, 50, NULL, NULL, hinst, NULL);

    SendMessage(hwEdit,WM_SETFONT,(WPARAM)hfont,0);

    if (winetest_interactive)
        ShowWindow (hwEdit, SW_SHOW);

    r = SendMessage(hwEdit, WM_CHAR, 'A', 1);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);
    r = SendMessage(hwEdit, WM_CHAR, 'B', 1);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);
    r = SendMessage(hwEdit, WM_CHAR, 'C', 1);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    GetWindowText(hwEdit, szLocalString, MAXLEN);
    ok(lstrcmp(szLocalString, "ABC")==0,
       "Wrong contents of edit: %s\n", szLocalString);

    r = SendMessage(hwEdit, EM_POSFROMCHAR,0,0);
    ok(r != -1,"EM_POSFROMCHAR failed index 0\n");
    r = SendMessage(hwEdit, EM_POSFROMCHAR,1,0);
    ok(r != -1,"EM_POSFROMCHAR failed index 1\n");
    r = SendMessage(hwEdit, EM_POSFROMCHAR,2,0);
    ok(r != -1,"EM_POSFROMCHAR failed index 2\n");
    r = SendMessage(hwEdit, EM_POSFROMCHAR,3,0);
    ok(r == -1,"EM_POSFROMCHAR succeeded index 3\n");

    DestroyWindow (hwEdit);
    DeleteObject(hfont);
}

START_TEST(edit)
{
    hinst = GetModuleHandleA(NULL);
    assert(RegisterWindowClasses());

    test_edit_control_1();
    test_edit_control_2();
    test_edit_control_3();
    test_edit_control_4();
    test_edit_control_5();
    test_edit_control_limittext();
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

    UnregisterWindowClasses();
}
