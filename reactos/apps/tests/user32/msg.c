/*
 * Unit tests for window message handling
 *
 * Copyright 1999 Ove Kaaven
 * Copyright 2003 Dimitrie O. Paun
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/test.h"


/*
FIXME: add tests for these
Window Edge Styles (Win31/Win95/98 look), in order of precedence:
 WS_EX_DLGMODALFRAME: double border, WS_CAPTION allowed
 WS_THICKFRAME: thick border
 WS_DLGFRAME: double border, WS_CAPTION not allowed (but possibly shown anyway)
 WS_BORDER (default for overlapped windows): single black border
 none (default for child (and popup?) windows): no border
*/

typedef enum { 
    sent=0x1, posted=0x2, parent=0x4, wparam=0x8, lparam=0x10,
    defwinproc=0x20
} msg_flags_t;

struct message {
    UINT message;          /* the WM_* code */
    msg_flags_t flags;     /* message props */
    WPARAM wParam;         /* expacted value of wParam */
    LPARAM lParam;         /* expacted value of lParam */
};

/* CreateWindow (for overlapped window, not initially visible) (16/32) */
static struct message WmCreateOverlappedSeq[] = {
    { WM_GETMINMAXINFO, sent },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { 0 }
};
/* ShowWindow (for overlapped window) (16/32) */
static struct message WmShowOverlappedSeq[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, /*FIXME: SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW*/ 0 },
    /* FIXME: WM_QUERYNEWPALETTE, if in 256-color mode */
    { WM_WINDOWPOSCHANGING, sent|wparam, /*FIXME: SWP_NOMOVE|SWP_NOSIZE*/ 0 },
    { WM_ACTIVATEAPP, sent|wparam, 1 },
    { WM_NCACTIVATE, sent|wparam, 1 },
    { WM_GETTEXT, sent|defwinproc },
    { WM_ACTIVATE, sent|wparam, 1 },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    { WM_NCPAINT, sent|wparam, 1 },
    { WM_GETTEXT, sent|defwinproc },
    { WM_ERASEBKGND, sent },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW },
    { WM_SIZE, sent },
    { WM_MOVE, sent },
    { 0 }
};

/* DestroyWindow (for overlapped window) (32) */
static struct message WmDestroyOverlappedSeq[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, 0 },
    { WM_NCACTIVATE, sent|wparam, 0 },
    { WM_ACTIVATE, sent|wparam, 0 },
    { WM_ACTIVATEAPP, sent|wparam, 0 },
    { WM_KILLFOCUS, sent|wparam, 0 },
    { WM_DESTROY, sent },
    { WM_NCDESTROY, sent },
    { 0 }
};
/* CreateWindow (for child window, not initially visible) */
static struct message WmCreateChildSeq[] = {
    { WM_NCCREATE, sent }, 
    /* child is inserted into parent's child list after WM_NCCREATE returns */
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { WM_SIZE, sent },
    { WM_MOVE, sent },
    { WM_PARENTNOTIFY, sent|parent|wparam, 1 },
    { 0 }
};
/* ShowWindow (for child window) */
static struct message WmShowChildSeq[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_ERASEBKGND, sent|parent },
    { WM_WINDOWPOSCHANGED, sent|wparam, 0 },
    { 0 }
};
/* DestroyWindow (for child window) */
static struct message WmDestroyChildSeq[] = {
    { WM_PARENTNOTIFY, sent|parent|wparam, 2 },
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_ERASEBKGND, sent|parent },
    { WM_WINDOWPOSCHANGED, sent|wparam, 0 },
    { WM_DESTROY, sent },
    { WM_NCDESTROY, sent },
    { 0 }
};
/* Moving the mouse in nonclient area */
static struct message WmMouseMoveInNonClientAreaSeq[] = { /* FIXME: add */
    { WM_NCHITTEST, sent },
    { WM_SETCURSOR, sent },
    { WM_NCMOUSEMOVE, posted },
    { 0 }
};
/* Moving the mouse in client area */
static struct message WmMouseMoveInClientAreaSeq[] = { /* FIXME: add */
    { WM_NCHITTEST, sent },
    { WM_SETCURSOR, sent },
    { WM_MOUSEMOVE, posted },
    { 0 }
};
/* Moving by dragging the title bar (after WM_NCHITTEST and WM_SETCURSOR) (outline move) */
static struct message WmDragTitleBarSeq[] = { /* FIXME: add */
    { WM_NCLBUTTONDOWN, sent|wparam, HTCAPTION },
    { WM_SYSCOMMAND, sent|defwinproc|wparam, SC_MOVE+2 },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_ENTERSIZEMOVE, sent|defwinproc },
    { WM_WINDOWPOSCHANGING, sent|defwinproc },
    { WM_WINDOWPOSCHANGED, sent|defwinproc },
    { WM_MOVE, sent|defwinproc },
    { WM_EXITSIZEMOVE, sent|defwinproc },
    { 0 }
};
/* Sizing by dragging the thick borders (after WM_NCHITTEST and WM_SETCURSOR) (outline move) */
static struct message WmDragThinkBordersBarSeq[] = { /* FIXME: add */
    { WM_NCLBUTTONDOWN, sent|wparam, 0xd },
    { WM_SYSCOMMAND, sent|defwinproc|wparam, 0xf004 },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_ENTERSIZEMOVE, sent|defwinproc },
    { WM_SIZING, sent|defwinproc|wparam, 4}, /* one for each mouse movement */
    { WM_WINDOWPOSCHANGING, sent|defwinproc },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_NCCALCSIZE, sent|defwinproc|wparam, 1 },
    { WM_NCPAINT, sent|defwinproc|wparam, 1 },
    { WM_GETTEXT, sent|defwinproc },
    { WM_ERASEBKGND, sent|defwinproc },
    { WM_WINDOWPOSCHANGED, sent|defwinproc },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc },
    { WM_EXITSIZEMOVE, sent|defwinproc },
    { 0 }
};
/* Resizing child window with MoveWindow (32) */
static struct message WmResizingChildWithMoveWindowSeq[] = {
    { WM_WINDOWPOSCHANGING, sent },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_ERASEBKGND, sent },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc },
    { 0 }
};
/* Clicking on inactive button */
static struct message WmClickInactiveButtonSeq[] = { /* FIXME: add */
    { WM_NCHITTEST, sent },
    { WM_PARENTNOTIFY, sent|parent|wparam, WM_LBUTTONDOWN },
    { WM_MOUSEACTIVATE, sent },
    { WM_MOUSEACTIVATE, sent|parent|defwinproc },
    { WM_SETCURSOR, sent },
    { WM_SETCURSOR, sent|parent|defwinproc },
    { WM_LBUTTONDOWN, posted },
    { WM_KILLFOCUS, posted|parent },
    { WM_SETFOCUS, posted },
    { WM_CTLCOLORBTN, posted|parent },
    { BM_SETSTATE, posted },
    { WM_CTLCOLORBTN, posted|parent },
    { WM_LBUTTONUP, posted },
    { BM_SETSTATE, posted },
    { WM_CTLCOLORBTN, posted|parent },
    { WM_COMMAND, posted|parent },
    { 0 }
};
/* Reparenting a button (16/32) */
/* The last child (button) reparented gets topmost for its new parent. */
static struct message WmReparentButtonSeq[] = { /* FIXME: add */
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER },
    { WM_ERASEBKGND, sent|parent },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOSIZE|SWP_NOZORDER },
    { WM_CHILDACTIVATE, sent },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOSIZE|SWP_NOREDRAW|SWP_NOZORDER },
    { WM_MOVE, sent|defwinproc },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { 0 }
};
/* Creation of a modal dialog (32) */
static struct message WmCreateModalDialogSeq[] = { /* FIXME: add */
    { WM_CANCELMODE, sent|parent },
    { WM_KILLFOCUS, sent|parent },
    { WM_ENABLE, sent|parent|wparam, 0 },
    /* (window proc creation messages not tracked yet, because...) */
    { WM_SETFONT, sent },
    { WM_INITDIALOG, sent },
    /* (...the window proc message hook was installed here, IsVisible still FALSE) */
    { WM_NCACTIVATE, sent|parent|wparam, 0 },
    { WM_GETTEXT, sent|defwinproc },
    { WM_ACTIVATE, sent|parent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_WINDOWPOSCHANGING, sent|parent },
    { WM_NCACTIVATE, sent|wparam, 1 },
    { WM_ACTIVATE, sent|wparam, 1 },
    /* (setting focus) */
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_NCPAINT, sent },
    { WM_GETTEXT, sent|defwinproc },
    { WM_ERASEBKGND, sent },
    { WM_CTLCOLORDLG, sent|defwinproc },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_PAINT, sent },
    /* FIXME: (bunch of WM_CTLCOLOR* for each control) */
    { WM_PAINT, sent|parent },
    { WM_ENTERIDLE, sent|parent|wparam, 0},
    { WM_SETCURSOR, sent|parent },
    { 0 }
};
/* Destruction of a modal dialog (32) */
static struct message WmDestroyModalDialogSeq[] = { /* FIXME: add */
    /* (inside dialog proc: EndDialog is called) */
    { WM_ENABLE, sent|parent|wparam, 1 },
    { WM_SETFOCUS, sent },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_NCPAINT, sent|parent },
    { WM_GETTEXT, sent|defwinproc },
    { WM_ERASEBKGND, sent|parent },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_NCACTIVATE, sent|wparam, 0 },
    { WM_ACTIVATE, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_WINDOWPOSCHANGING, sent|parent },
    { WM_NCACTIVATE, sent|parent|wparam, 1 },
    { WM_GETTEXT, sent|defwinproc },
    { WM_ACTIVATE, sent|parent|wparam, 1 },
    { WM_KILLFOCUS, sent },
    { WM_SETFOCUS, sent|parent },
    { WM_DESTROY, sent },
    { WM_NCDESTROY, sent },
    { 0 }
};
/* Creation of a modal dialog that is resized inside WM_INITDIALOG (32) */
static struct message WmCreateModalDialogResizeSeq[] = { /* FIXME: add */
    /* (inside dialog proc, handling WM_INITDIALOG) */
    { WM_WINDOWPOSCHANGING, sent },
    { WM_NCCALCSIZE, sent },
    { WM_NCACTIVATE, sent|parent|wparam, 0 },
    { WM_GETTEXT, sent|defwinproc },
    { WM_ACTIVATE, sent|parent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_WINDOWPOSCHANGING, sent|parent },
    { WM_NCACTIVATE, sent|wparam, 1 },
    { WM_ACTIVATE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_SIZE, sent|defwinproc },
    /* (setting focus) */
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_NCPAINT, sent },
    { WM_GETTEXT, sent|defwinproc },
    { WM_ERASEBKGND, sent },
    { WM_CTLCOLORDLG, sent|defwinproc },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_PAINT, sent },
    /* (bunch of WM_CTLCOLOR* for each control) */
    { WM_PAINT, sent|parent },
    { WM_ENTERIDLE, sent|parent|wparam, 0 },
    { WM_SETCURSOR, sent|parent },
    { 0 }
};

static int sequence_cnt, sequence_size;
static struct message* sequence;

static void add_message(struct message msg)
{
    if (!sequence) 
	sequence = malloc ( (sequence_size = 10) * sizeof (struct message) );
    if (sequence_cnt == sequence_size) 
	sequence = realloc ( sequence, (sequence_size *= 2) * sizeof (struct message) );
    assert(sequence);
    sequence[sequence_cnt++] = msg;
}

static void flush_sequence()
{
    free(sequence);
    sequence = 0;
    sequence_cnt = sequence_size = 0;
}

static void ok_sequence(struct message *expected, const char *context)
{
    static struct message end_of_sequence = { 0, 0, 0, 0 };
    struct message *actual = sequence;
    
    add_message(end_of_sequence);

    /* naive sequence comparison. Would be nice to use a regexp engine here */
    while (expected->message || actual->message)
    {
	if (expected->message == actual->message)
	{
	    if (expected->flags & wparam)
		 ok (expected->wParam == actual->wParam,
		     "%s: in msg 0x%04x expecting wParam 0x%x got 0x%x\n",
		     context, expected->message, expected->wParam, actual->wParam);
	    if (expected->flags & lparam)
		 ok (expected->lParam == actual->lParam,
		     "%s: in msg 0x%04x expecting lParam 0x%lx got 0x%lx\n",
		     context, expected->message, expected->lParam, actual->lParam);
	    /* FIXME: should we check defwinproc? */
	    ok ((expected->flags & (sent|posted)) == (actual->flags & (sent|posted)),
		"%s: the msg 0x%04x should have been %s\n",
		context, expected->message, (expected->flags & posted) ? "posted" : "sent");
	    ok ((expected->flags & parent) == (actual->flags & parent),
		"%s: the msg 0x%04x was expected in %s\n",
		context, expected->message, (expected->flags & parent) ? "parent" : "child");
	    expected++;
	    actual++;
	}
	else if (expected->message && ((expected + 1)->message == actual->message) )
	{
	  todo_wine {
	    ok (FALSE, "%s: the msg 0x%04x was not received\n", context, expected->message);
	    expected++;
	  }
	}
	else if (actual->message && (expected->message == (actual + 1)->message) )
	{
	  todo_wine {
	    ok (FALSE, "%s: the msg 0x%04x was not expected\n", context, actual->message);
	    actual++;
	  }
	}
	else
	{
	  todo_wine {
	    ok (FALSE, "%s: the msg 0x%04x was expected, but got msg 0x%04x instead\n",
		context, expected->message, actual->message);
	    expected++;
	    actual++;
	  }
	}
    }

    flush_sequence();
}

/* test if we receive the right sequence of messages */
static void test_messages(void)
{
    HWND hwnd, hparent, hchild;
    HWND hchild2, hbutton;

    hwnd = CreateWindowExA(0, "TestWindowClass", "Test overlapped", WS_OVERLAPPEDWINDOW,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hwnd != 0, "Failed to create overlapped window\n");
    ok_sequence(WmCreateOverlappedSeq, "CreateWindow:overlapped");
    
    ShowWindow(hwnd, TRUE);
    ok_sequence(WmShowOverlappedSeq, "ShowWindow:overlapped");

    DestroyWindow(hwnd);
    ok_sequence(WmDestroyOverlappedSeq, "DestroyWindow:overlapped");

    hparent = CreateWindowExA(0, "TestParentClass", "Test parent", WS_OVERLAPPEDWINDOW,
                              100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hparent != 0, "Failed to create parent window\n");
    flush_sequence();

    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILDWINDOW,
                             0, 0, 10, 10, hparent, 0, 0, NULL);
    ok (hchild != 0, "Failed to create child window\n");
    ok_sequence(WmCreateChildSeq, "CreateWindow:child");
    
    hchild2 = CreateWindowExA(0, "SimpleWindowClass", "Test child2", WS_CHILDWINDOW,
                               100, 100, 50, 50, hparent, 0, 0, NULL);
    ok (hchild2 != 0, "Failed to create child2 window\n");
    flush_sequence();

    hbutton = CreateWindowExA(0, "TestWindowClass", "Test button", WS_CHILDWINDOW,
                              0, 100, 50, 50, hchild, 0, 0, NULL);
    ok (hbutton != 0, "Failed to create button window\n");
    flush_sequence();

    ShowWindow(hchild, TRUE);
    ok_sequence(WmShowChildSeq, "ShowWindow:child");

    MoveWindow(hchild, 10, 10, 20, 20, TRUE);
    ok_sequence(WmResizingChildWithMoveWindowSeq, "MoveWindow:child");

    DestroyWindow(hchild);
    ok_sequence(WmDestroyChildSeq, "DestroyWindow:child");
}

static LRESULT WINAPI MsgCheckProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct message msg = { message, sent|wparam|lparam, wParam, lParam };

    add_message(msg);
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static BOOL RegisterWindowClasses(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = MsgCheckProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "TestWindowClass";

    if(!RegisterClassA(&cls)) return FALSE;

    cls.style = 0;
    cls.lpfnWndProc = DefWindowProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "TestParentClass";

    if(!RegisterClassA(&cls)) return FALSE;

    cls.style = 0;
    cls.lpfnWndProc = DefWindowProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "SimpleWindowClass";

    if(!RegisterClassA(&cls)) return FALSE;

    return TRUE;
}

START_TEST(msg)
{
    if (!RegisterWindowClasses()) assert(0);

    test_messages();
}
