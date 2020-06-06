/* Unit test suite for the dialog functions.
 *
 * Copyright 2004 Bill Medland
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
 *
 *
 *
 * This test suite currently works by building a quite complex hierarchy of
 * objects in a variety of styles and then performs a limited number of tests
 * for the previous and next dialog group or tab items.
 *
 * The test specifically does not test all possibilities at this time since
 * there are several cases where the Windows behaviour is rather strange and
 * significant work would be required to get the Wine code to duplicate the
 * strangeness, especially since most are in situations that would not
 * normally be met.
 */

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"

#define MAXHWNDS 1024
static HWND hwnd [MAXHWNDS];
static unsigned int numwnds=1; /* 0 is reserved for null */

/* Global handles */
static HINSTANCE g_hinst;                          /* This application's HINSTANCE */
static HWND g_hwndMain, g_hwndButton1, g_hwndButton2, g_hwndButtonCancel;
static HWND g_hwndTestDlg, g_hwndTestDlgBut1, g_hwndTestDlgBut2, g_hwndTestDlgEdit;
static HWND g_hwndInitialFocusT1, g_hwndInitialFocusT2, g_hwndInitialFocusGroupBox;

static LONG g_styleInitialFocusT1, g_styleInitialFocusT2;
static BOOL g_bInitialFocusInitDlgResult, g_bReceivedCommand;

static BOOL g_terminated;
static BOOL g_button1Clicked;

typedef struct {
    INT_PTR id;
    int parent;
    DWORD style;
    DWORD exstyle;
} h_entry;

static const h_entry hierarchy [] = {
    /* 0 is reserved for the null window */
    {  1,  0, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE},
    { 20,  1,  WS_CHILD | WS_VISIBLE | WS_GROUP, 0},
    {  2,  1,  WS_CHILD | WS_VISIBLE, WS_EX_CONTROLPARENT},
    { 60,  2,  WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0},
    /* What happens with groups when the parent is disabled */
    {  8,  2,  WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP, WS_EX_CONTROLPARENT},
    { 85,  8,  WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP, 0},
    {  9,  8,  WS_CHILD, WS_EX_CONTROLPARENT},
    { 86,  9,  WS_CHILD | WS_VISIBLE, 0},
    { 87,  9,  WS_CHILD | WS_VISIBLE, 0},
    { 31,  8,  WS_CHILD | WS_VISIBLE | WS_GROUP, 0},
    { 10,  2,  WS_CHILD | WS_VISIBLE, WS_EX_CONTROLPARENT},
    { 88, 10,  WS_CHILD | WS_VISIBLE | WS_GROUP, 0},
    { 11, 10,  WS_CHILD, WS_EX_CONTROLPARENT},
    { 89, 11,  WS_CHILD | WS_VISIBLE, 0},
    { 32, 11,  WS_CHILD | WS_VISIBLE | WS_GROUP, 0},
    { 90, 11,  WS_CHILD | WS_VISIBLE, 0},
    { 33, 10,  WS_CHILD | WS_VISIBLE | WS_GROUP, 0},
    { 21,  2,  WS_CHILD | WS_VISIBLE | WS_GROUP, 0},
    { 61,  2,  WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0},
    {  3,  1,  WS_CHILD | WS_VISIBLE | DS_CONTROL, 0},
    { 22,  3,  WS_CHILD | WS_VISIBLE | WS_GROUP, 0},
    { 62,  3,  WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0},
    {  7,  3,  WS_CHILD | WS_VISIBLE, WS_EX_CONTROLPARENT},
    {  4,  7,  WS_CHILD | WS_VISIBLE | DS_CONTROL, 0},
    { 83,  4,  WS_CHILD | WS_VISIBLE, 0},
    {  5,  4,  WS_CHILD | WS_VISIBLE | DS_CONTROL, 0},
    /* A couple of controls around the main dialog */
    { 29,  5,  WS_CHILD | WS_VISIBLE | WS_GROUP, 0},
    { 81,  5,  WS_CHILD | WS_VISIBLE, 0},
    /* The main dialog with lots of controls */
    {  6,  5,  WS_CHILD | WS_VISIBLE, WS_EX_CONTROLPARENT},
        /* At the start of a dialog */
        /* Disabled controls are skipped */
    { 63,  6,  WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP, 0},
        /* Invisible controls are skipped */
    { 64,  6,  WS_CHILD | WS_TABSTOP, 0},
        /* Invisible disabled controls are skipped */
    { 65,  6,  WS_CHILD | WS_DISABLED | WS_TABSTOP, 0},
        /* Non-tabstop controls are skipped for tabs but not for groups */
    { 66,  6,  WS_CHILD | WS_VISIBLE, 0},
        /* End of first group, with no tabstops in it */
    { 23,  6,  WS_CHILD | WS_VISIBLE | WS_GROUP, 0},
        /* At last a tabstop */
    { 67,  6,  WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0},
    /* A group that is totally disabled or invisible */
    { 24,  6,  WS_CHILD | WS_DISABLED | WS_GROUP, 0},
    { 68,  6,  WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP, 0},
    { 69,  6,  WS_CHILD | WS_TABSTOP, 0},
    /* A valid group in the middle of the dialog (not the first nor last group*/
    { 25,  6,  WS_CHILD | WS_VISIBLE | WS_GROUP, 0},
        /* A non-tabstop item will be skipped for tabs */
    { 70,  6,  WS_CHILD | WS_VISIBLE, 0},
        /* A disabled item will be skipped for tabs and groups */
    { 71,  6,  WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP, 0},
        /* A valid item will be found for tabs and groups */
    { 72,  6,  WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0},
        /* A disabled item to skip when looking for the next group item */
    { 73,  6,  WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP, 0},
    /* The next group begins with an enabled visible label */
    { 26,  6,  WS_CHILD | WS_VISIBLE | WS_GROUP, 0},
    { 74,  6,  WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0},
    { 75,  6,  WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0},
    /* That group is terminated by a disabled label */
    { 27,  6,  WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_GROUP, 0},
    { 76,  6,  WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0},
    { 77,  6,  WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0},
    /* That group is terminated by an invisible label */
    { 28,  6,  WS_CHILD | WS_GROUP, 0},
    /* The end of the dialog with item for loop and recursion testing */
    { 78,  6,  WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0},
    /* No tabstop so skipped for prev tab, but found for prev group */
    { 79,  6,  WS_CHILD | WS_VISIBLE, 0},
    { 80,  6,  WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP, 0},
    /* A couple of controls after the main dialog */
    { 82,  5,  WS_CHILD | WS_VISIBLE, 0},
    { 30,  5,  WS_CHILD | WS_VISIBLE | WS_GROUP, 0},
    /* And around them */
    { 84,  4,  WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0},
    {0, 0, 0, 0}
};

static DWORD get_button_style(HWND button)
{
    return GetWindowLongW(button, GWL_STYLE) & BS_TYPEMASK;
}

static BOOL CreateWindows (HINSTANCE hinst)
{
    const h_entry *p = hierarchy;

    while (p->id != 0)
    {
        DWORD style, exstyle;
        char ctrlname[16];

        /* Basically assert that the hierarchy is valid and track the
         * maximum control number
         */
        if (p->id >= numwnds)
        {
            if (p->id >=  ARRAY_SIZE(hwnd))
            {
                trace ("Control %ld is out of range\n", p->id);
                return FALSE;
            }
            else
                numwnds = p->id+1;
        }
        if (p->id <= 0)
        {
            trace ("Control %ld is out of range\n", p->id);
            return FALSE;
        }
        if (hwnd[p->id] != 0)
        {
            trace ("Control %ld is used more than once\n", p->id);
            return FALSE;
        }

        /* Create the control */
        sprintf (ctrlname, "ctrl%4.4ld", p->id);
        hwnd[p->id] = CreateWindowExA(p->exstyle, p->parent ? "static" : "GetNextDlgItemWindowClass", ctrlname, p->style, 10, 10, 10, 10, hwnd[p->parent], p->parent ? (HMENU) (2000 + p->id) : 0, hinst, 0);
        if (!hwnd[p->id])
        {
            trace ("Failed to create control %ld\n", p->id);
            return FALSE;
        }

        /* Check that the styles are as we specified (except the main one
         * which is quite frequently messed up).  If this keeps breaking then
         * we could mask out the bits that don't concern us.
         */
        if (p->parent)
        {
            style = GetWindowLongA(hwnd[p->id], GWL_STYLE);
            exstyle = GetWindowLongA(hwnd[p->id], GWL_EXSTYLE);
            if (style != p->style || exstyle != p->exstyle)
            {
                trace ("Style mismatch at %ld: %8.8x %8.8x cf %8.8x %8.8x\n", p->id, style, exstyle, p->style, p->exstyle);
            }
        }
        p++;
    }

    return TRUE;
}

/* Form the lParam of a WM_KEYDOWN message */
static DWORD KeyDownData (int repeat, int scancode, int extended, int wasdown)
{
    return ((repeat & 0x0000FFFF) | ((scancode & 0x00FF) << 16) |
            (extended ? 0x01000000 : 0) | (wasdown ? 0x40000000 : 0));
}

/* Form a WM_KEYDOWN VK_TAB message to the specified window */
static void FormTabMsg (MSG *pMsg, HWND hwnd)
{
    pMsg->hwnd = hwnd;
    pMsg->message = WM_KEYDOWN;
    pMsg->wParam = VK_TAB;
    pMsg->lParam = KeyDownData (1, 0x0F, 0, 0);
    /* pMsg->time is not set.  It shouldn't be needed */
    /* pMsg->pt is ignored */
}

/* Form a WM_KEYDOWN VK_RETURN message to the specified window */
static void FormEnterMsg (MSG *pMsg, HWND hwnd)
{
    pMsg->hwnd = hwnd;
    pMsg->message = WM_KEYDOWN;
    pMsg->wParam = VK_RETURN;
    pMsg->lParam = KeyDownData (1, 0x1C, 0, 0);
    /* pMsg->time is not set.  It shouldn't be needed */
    /* pMsg->pt is ignored */
}

/***********************************************************************
 *
 * The actual tests
 */

typedef struct
{
    int isok; /* or is it todo */
    int test;
    int dlg;
    int ctl;
    int tab;
    int prev;
    int res;
} test_record;

static int id (HWND h)
{
    unsigned int i;
    for (i = 0; i < numwnds; i++)
        if (hwnd[i] == h)
            return i;
    return -1;
}

/* Tests
 *
 * Tests 1-8 test the hCtl argument of null or the dialog itself.
 *
 *   1. Prev Group of null is null
 *   2. Prev Tab of null is null
 *   3. Prev Group of hDlg in hDlg is null
 *   4. Prev Tab of hDlg in hDlg is null
 *   5. Next Group of null is first visible enabled child
 *      Check it skips invisible, disabled and both.
 *   6. Next Tab of null is first visible enabled tabstop
 *      Check it skips invisible, disabled, nontabstop, and in combination.
 *   7. Next Group of hDlg in hDlg is as of null
 *   8. Next Tab of hDlg in hDlg is as of null
 *
 * Tests 9-14 test descent
 *
 *   9. DS_CONTROL does not result in descending the hierarchy for Tab Next
 *  10. DS_CONTROL does not result in descending the hierarchy for Group Next
 *  11. WS_EX_CONTROLPARENT results in descending the hierarchy for Tab Next
 *  12. WS_EX_CONTROLPARENT results in descending the hierarchy for Group Next
 *  13. WS_EX_CONTROLPARENT results in descending the hierarchy for Tab Prev
 *  14. WS_EX_CONTROLPARENT results in descending the hierarchy for Group Prev
 *
 * Tests 15-24 are the basic Prev/Next Group tests
 *
 *  15. Next Group of a visible enabled non-group control is the next visible
 *      enabled non-group control, if there is one before the next group
 *  16. Next Group of a visible enabled non-group control wraps around to the
 *      beginning of the group on finding a control that starts another group.
 *      Note that the group is in the middle of the dialog.
 *  17. As 16 except note that the next group is started with a disabled
 *      visible control.
 *  18. As 16 except note that the next group is started with an invisible
 *      enabled control.
 *  19. Next Group wraps around the controls of the dialog
 *  20. Next Group is the same even if the initial control is disabled.
 *  21. Next Group is the same even if the initial control is invisible.
 *  22. Next Group is the same even if the initial control has the group style
 *  23. Next Group returns the initial control if there is no visible enabled
 *      control in the group. (Initial control disabled and not group style).
 *  24. Prev version of test 16.
 *      Prev Group of a visible enabled non-group control wraps around to the
 *      beginning of the group on finding a control that starts the group.
 *      Note that the group is in the middle of the dialog.
 *
 * In tests 25 to 28 the control is sitting under dialogs which do not have
 * the WS_EX_CONTROLPARENT style and so cannot be reached from the top of
 * the dialog.
 *
 *  25. Next Group of an inaccessible control is as if it were accessible
 *  26. Prev Group of an inaccessible control begins searching at the highest
 *      level ancestor that did not permit recursion down the hierarchy
 *  27. Next Tab of an inaccessible control is as if it were accessible
 *  28. Prev Tab of an inaccessible control begins searching at the highest
 *      level ancestor that did not permit recursion down the hierarchy.
 *
 * Tests 29- are the basic Tab tests
 *
 *  29. Next Tab of a control is the next visible enabled control with the
 *      Tabstop style (N.B. skips disabled, invisible and non-tabstop)
 *  30. Prev Tab of a control is the previous visible enabled control with the
 *      Tabstop style (N.B. skips disabled, invisible and non-tabstop)
 *  31. Next Tab test with at least two layers of descent and finding the
 *      result not at the first control.
 *  32. Next Tab test with at least two layers of descent with the descent and
 *      control at the start of each level.
 *  33. Prev Tab test with at least two layers of descent and finding the
 *      result not at the last control.
 *  34. Prev Tab test with at least two layers of descent with the descent and
 *      control at the end of each level.
 *
 *  35. Passing NULL may result in the first child being the one returned.
 *      (group test)
 *  36. Passing NULL may result in the first child being the one returned.
 *      (tab test)
 */

static void test_GetNextDlgItem(void)
{
    static test_record test [] =
    {
        /* isok test dlg  ctl  tab  prev res  */

        {   1,   1,    6,   0,   0,   1,   0},
        {   1,   2,    6,   0,   1,   1,   0},
        {   1,   3,    6,   6,   0,   1,   0},
        {   1,   4,    6,   6,   1,   1,   0},
        {   1,   5,    6,   0,   0,   0,  66},
        {   1,   6,    6,   0,   1,   0,  67},
        {   1,   7,    6,   6,   0,   0,  66},
        {   1,   8,    6,   6,   1,   0,  67},

        {   1,   9,    4,  83,   1,   0,  84},
        {   1,  10,    4,  83,   0,   0,   5},
        {   1,  11,    5,  81,   1,   0,  67},
        {   1,  12,    5,  81,   0,   0,  66},
        {   1,  13,    5,  82,   1,   1,  78},

        {   1,  14,    5,  82,   0,   1,  79},
        {   1,  15,    6,  70,   0,   0,  72},
        {   1,  16,    6,  72,   0,   0,  25},
        {   1,  17,    6,  75,   0,   0,  26},
        {   1,  18,    6,  77,   0,   0,  76},
        {   1,  19,    6,  79,   0,   0,  66},
        {   1,  20,    6,  71,   0,   0,  72},
        {   1,  21,    6,  64,   0,   0,  66},

        {   1,  22,    6,  25,   0,   0,  70},
        {   1,  23,    6,  68,   0,   0,  68},
        {   1,  24,    6,  25,   0,   1,  72},
        {   1,  25,    1,  70,   0,   0,  72},
        /*{   0,  26,    1,  70,   0,   1,   3}, Crashes Win95*/
        {   1,  27,    1,  70,   1,   0,  72},
        /*{   0,  28,    1,  70,   1,   1,  61}, Crashes Win95*/

        {   1,  29,    6,  67,   1,   0,  72},
        {   1,  30,    6,  72,   1,   1,  67},

        {   1,  35,    2,   0,   0,   0,  60},
        {   1,  36,    2,   0,   1,   0,  60},

        {   0,   0,    0,   0,   0,   0,   0}  /* End of test */
    };
    const test_record *p = test;

    ok (CreateWindows (g_hinst), "Could not create test windows\n");

    while (p->dlg)
    {
        HWND a;
        a = (p->tab ? GetNextDlgTabItem : GetNextDlgGroupItem) (hwnd[p->dlg], hwnd[p->ctl], p->prev);
        todo_wine_if (!p->isok)
            ok (a == hwnd[p->res], "Test %d: %s %s item of %d in %d was %d instead of %d\n", p->test, p->prev ? "Prev" : "Next", p->tab ? "Tab" : "Group", p->ctl, p->dlg, id(a), p->res);
        p++;
    }
}

/*
 *  OnMainWindowCreate
 */
static BOOL OnMainWindowCreate(HWND hwnd, LPCREATESTRUCTA lpcs)
{
    g_hwndButton1 = CreateWindowA("button", "Button &1",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON | BS_TEXT,
            10, 10, 80, 80, hwnd, (HMENU)100, g_hinst, 0);
    if (!g_hwndButton1) return FALSE;

    g_hwndButton2 = CreateWindowA("button", "Button &2",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_TEXT,
            110, 10, 80, 80, hwnd, (HMENU)200, g_hinst, 0);
    if (!g_hwndButton2) return FALSE;

    g_hwndButtonCancel = CreateWindowA("button", "Cancel",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_TEXT,
            210, 10, 80, 80, hwnd, (HMENU)IDCANCEL, g_hinst, 0);
    if (!g_hwndButtonCancel) return FALSE;

    return TRUE;
}


/*
 *  OnTestDlgCreate
 */

static BOOL OnTestDlgCreate (HWND hwnd, LPCREATESTRUCTA lpcs)
{
    g_hwndTestDlgEdit = CreateWindowExA( WS_EX_LEFT | WS_EX_LTRREADING |
            WS_EX_RIGHTSCROLLBAR | WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE,
            "Edit", "Edit",
            WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL,
            16,33,184,24, hwnd, (HMENU)101, g_hinst, 0);
    if (!g_hwndTestDlgEdit) return FALSE;

    g_hwndTestDlgBut1 = CreateWindowExA( WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR
            | WS_EX_NOPARENTNOTIFY,
            "button", "Button &1",
            WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_TEXT,
            204,33,30,24, hwnd, (HMENU)201, g_hinst, 0);
    if (!g_hwndTestDlgBut1) return FALSE;

    g_hwndTestDlgBut2 = CreateWindowExA( WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR
            | WS_EX_NOPARENTNOTIFY, "button",
            "Button &2",
            WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_TEXT,
            90,102,80,24, hwnd, (HMENU)IDCANCEL, g_hinst, 0);
    if (!g_hwndTestDlgBut2) return FALSE;

    return TRUE;
}

static LRESULT CALLBACK main_window_procA (HWND hwnd, UINT uiMsg, WPARAM wParam,
        LPARAM lParam)
{
    switch (uiMsg)
    {
        /* Add blank case statements for these to ensure we don't use them
         * by mistake.
         */
        case DM_GETDEFID: break;
        case DM_SETDEFID: break;

        case WM_CREATE:
            return (OnMainWindowCreate (hwnd,
                    (LPCREATESTRUCTA) lParam) ? 0 : (LRESULT) -1);
        case WM_COMMAND:
            if (wParam == IDCANCEL)
            {
                g_terminated = TRUE;
                return 0;
            }
            else if ((wParam == 100 || wParam == 0xFFFF) && lParam)
            {
                g_button1Clicked = TRUE;
            }
            break;
    }

    return DefWindowProcA (hwnd, uiMsg, wParam, lParam);
}

static LRESULT CALLBACK disabled_test_proc (HWND hwnd, UINT uiMsg,
        WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg)
    {
        case WM_INITDIALOG:
        {
            DWORD dw;
            HWND hwndOk;

            dw = SendMessageA(hwnd, DM_GETDEFID, 0, 0);
            assert(DC_HASDEFID == HIWORD(dw));
            hwndOk = GetDlgItem(hwnd, LOWORD(dw));
            assert(hwndOk);
            EnableWindow(hwndOk, FALSE);

            PostMessageA(hwnd, WM_KEYDOWN, VK_RETURN, 0);
            PostMessageA(hwnd, WM_COMMAND, IDCANCEL, 0);
            break;
        }
        case WM_COMMAND:
            if (wParam == IDOK)
            {
                g_terminated = TRUE;
                EndDialog(hwnd, 0);
                return 0;
            }
            else if (wParam == IDCANCEL)
            {
                EndDialog(hwnd, 0);
                return 0;
            }
            break;
    }

    return DefWindowProcA (hwnd, uiMsg, wParam, lParam);
}

static LRESULT CALLBACK testDlgWinProc (HWND hwnd, UINT uiMsg, WPARAM wParam,
        LPARAM lParam)
{
    switch (uiMsg)
    {
        /* Add blank case statements for these to ensure we don't use them
         * by mistake.
         */
        case DM_GETDEFID: break;
        case DM_SETDEFID: break;

        case WM_CREATE:
            return (OnTestDlgCreate (hwnd,
                    (LPCREATESTRUCTA) lParam) ? 0 : (LRESULT) -1);
        case WM_COMMAND:
            if(LOWORD(wParam) == 300) g_bReceivedCommand = TRUE;
    }

    return DefDlgProcA (hwnd, uiMsg, wParam, lParam);
}

static LRESULT CALLBACK test_control_procA(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(msg)
    {
        case WM_CREATE:
        {
            static const short sample[] = { 10,1,2,3,4,5 };
            CREATESTRUCTA *cs = (CREATESTRUCTA *)lparam;
            short *data = cs->lpCreateParams;
            ok(!memcmp(data, sample, sizeof(sample)), "data mismatch: %d,%d,%d,%d,%d\n", data[0], data[1], data[2], data[3], data[4]);
        }
        return 0;

    default:
        break;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static BOOL RegisterWindowClasses (void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = DefWindowProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = g_hinst;
    cls.hIcon = NULL;
    cls.hCursor = LoadCursorA (NULL, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "GetNextDlgItemWindowClass";

    if (!RegisterClassA (&cls)) return FALSE;

    cls.lpfnWndProc = main_window_procA;
    cls.lpszClassName = "IsDialogMessageWindowClass";
    if (!RegisterClassA (&cls)) return FALSE;

    cls.lpfnWndProc = test_control_procA;
    cls.lpszClassName = "TESTCONTROL";
    if (!RegisterClassA (&cls)) return FALSE;

    GetClassInfoA(0, "#32770", &cls);
    cls.lpfnWndProc = testDlgWinProc;
    cls.lpszClassName = "WM_NEXTDLGCTLWndClass";
    if (!RegisterClassA (&cls)) return FALSE;

    return TRUE;
}

static void test_WM_NEXTDLGCTL(void)
{
    HWND child1, child2, child3;
    MSG msg;
    DWORD dwVal;

    g_hwndTestDlg = CreateWindowExA( WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR
              | WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT | WS_EX_APPWINDOW,
              "WM_NEXTDLGCTLWndClass",
              "WM_NEXTDLGCTL Message test window",
              WS_POPUPWINDOW | WS_CLIPSIBLINGS | WS_DLGFRAME | WS_OVERLAPPED |
              WS_MINIMIZEBOX | WS_MAXIMIZEBOX | DS_3DLOOK | DS_SETFONT | DS_MODALFRAME,
              0, 0, 235, 135,
              NULL, NULL, g_hinst, 0);

    assert (g_hwndTestDlg);
    assert (g_hwndTestDlgBut1);
    assert (g_hwndTestDlgBut2);
    assert (g_hwndTestDlgEdit);

    /*
     * Test message DM_SETDEFID
     */

    DefDlgProcA( g_hwndTestDlg, DM_SETDEFID, IDCANCEL, 0 );
    DefDlgProcA( g_hwndTestDlgBut1, BM_SETSTYLE, BS_DEFPUSHBUTTON, FALSE );
    dwVal = DefDlgProcA(g_hwndTestDlg, DM_GETDEFID, 0, 0);

    ok ( IDCANCEL == (LOWORD(dwVal)), "Did not set default ID\n" );

    /*
     * Check whether message WM_NEXTDLGCTL is changing the focus to next control and if
     * the destination control is a button, style of the button should be changed to
     * BS_DEFPUSHBUTTON with out making it default.
     */

    /* Keep the focus on Edit control. */
    SetFocus(g_hwndTestDlgEdit);
    ok((GetFocus() == g_hwndTestDlgEdit), "Focus didn't set on Edit control\n");

    /* Test message WM_NEXTDLGCTL */
    DefDlgProcA(g_hwndTestDlg, WM_NEXTDLGCTL, 0, 0);
    ok((GetFocus() == g_hwndTestDlgBut1), "Focus didn't move to first button\n");

    /* Check whether the default button ID got changed by sending message "WM_NEXTDLGCTL" */
    dwVal = DefDlgProcA(g_hwndTestDlg, DM_GETDEFID, 0, 0);
    ok(IDCANCEL == (LOWORD(dwVal)), "WM_NEXTDLGCTL changed default button\n");

    /*
     * Check whether the style of the button which got the focus, changed to BS_DEFPUSHBUTTON and
     * the style of default button changed to BS_PUSHBUTTON.
     */
    ok(get_button_style(g_hwndTestDlgBut1) == BS_DEFPUSHBUTTON, "Button1's style not set to BS_DEFPUSHBUTTON");
    ok(get_button_style(g_hwndTestDlgBut2) == BS_PUSHBUTTON, "Button2's style not set to BS_PUSHBUTTON");

    /* Move focus to Button2 using "WM_NEXTDLGCTL" */
    DefDlgProcA(g_hwndTestDlg, WM_NEXTDLGCTL, 0, 0);
    ok((GetFocus() == g_hwndTestDlgBut2), "Focus didn't move to second button\n");

    /* Check whether the default button ID got changed by sending message "WM_NEXTDLGCTL" */
    dwVal = DefDlgProcA(g_hwndTestDlg, DM_GETDEFID, 0, 0);
    ok(IDCANCEL == (LOWORD(dwVal)), "WM_NEXTDLGCTL changed default button\n");

    /*
     * Check whether the style of the button which got the focus, changed to BS_DEFPUSHBUTTON and
     * the style of button which lost the focus changed to BS_PUSHBUTTON.
     */
    ok(get_button_style(g_hwndTestDlgBut1) == BS_PUSHBUTTON, "Button1's style not set to BS_PUSHBUTTON");
    ok(get_button_style(g_hwndTestDlgBut2) == BS_DEFPUSHBUTTON, "Button2's style not set to BS_DEFPUSHBUTTON");

    /* Move focus to Edit control using "WM_NEXTDLGCTL" */
    DefDlgProcA(g_hwndTestDlg, WM_NEXTDLGCTL, 0, 0);
    ok((GetFocus() == g_hwndTestDlgEdit), "Focus didn't move to Edit control\n");

    /* Check whether the default button ID got changed by sending message "WM_NEXTDLGCTL" */
    dwVal = DefDlgProcA(g_hwndTestDlg, DM_GETDEFID, 0, 0);
    ok(IDCANCEL == (LOWORD(dwVal)), "WM_NEXTDLGCTL changed default button\n");

    /* test nested default buttons */

    child1 = CreateWindowA("button", "child1", WS_VISIBLE|WS_CHILD, 0, 0, 50, 50,
                           g_hwndTestDlg, (HMENU)100, g_hinst, NULL);
    ok(child1 != NULL, "failed to create first child\n");
    child2 = CreateWindowA("button", "child2", WS_VISIBLE|WS_CHILD, 60, 60, 30, 30,
                           g_hwndTestDlg, (HMENU)200, g_hinst, NULL);
    ok(child2 != NULL, "failed to create second child\n");
    /* create nested child */
    child3 = CreateWindowA("button", "child3", WS_VISIBLE|WS_CHILD, 10, 10, 10, 10,
                           child1, (HMENU)300, g_hinst, NULL);
    ok(child3 != NULL, "failed to create subchild\n");

    DefDlgProcA( g_hwndTestDlg, DM_SETDEFID, 200, 0);
    dwVal = DefDlgProcA( g_hwndTestDlg, DM_GETDEFID, 0, 0);
    ok(LOWORD(dwVal) == 200, "expected 200, got %x\n", dwVal);

    DefDlgProcA( g_hwndTestDlg, DM_SETDEFID, 300, 0);
    dwVal = DefDlgProcA( g_hwndTestDlg, DM_GETDEFID, 0, 0);
    ok(LOWORD(dwVal) == 300, "expected 300, got %x\n", dwVal);
    ok(SendMessageW( child3, WM_GETDLGCODE, 0, 0) != DLGC_DEFPUSHBUTTON,
       "expected child3 not to be marked as DLGC_DEFPUSHBUTTON\n");

    g_bReceivedCommand = FALSE;
    FormEnterMsg (&msg, child3);
    ok(IsDialogMessageA(g_hwndTestDlg, &msg), "Did not handle the ENTER\n");
    ok(g_bReceivedCommand, "Did not trigger the default Button action\n");

    DestroyWindow(child3);
    DestroyWindow(child2);
    DestroyWindow(child1);
    DestroyWindow(g_hwndTestDlg);
}

static LRESULT CALLBACK hook_proc(INT code, WPARAM wParam, LPARAM lParam)
{
    ok(0, "unexpected hook called, code %d\n", code);
    return CallNextHookEx(NULL, code, wParam, lParam);
}

static BOOL g_MSGF_DIALOGBOX;
static LRESULT CALLBACK hook_proc2(INT code, WPARAM wParam, LPARAM lParam)
{
    ok(code == MSGF_DIALOGBOX, "unexpected hook called, code %d\n", code);
    g_MSGF_DIALOGBOX = code == MSGF_DIALOGBOX;
    return CallNextHookEx(NULL, code, wParam, lParam);
}

static void test_IsDialogMessage(void)
{
    HHOOK hook;
    MSG msg;

    g_hwndMain = CreateWindowA("IsDialogMessageWindowClass", "IsDialogMessageWindowClass",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, g_hinst, 0);

    assert (g_hwndMain);
    assert (g_hwndButton1);
    assert (g_hwndButtonCancel);

    if (0)
    {
        /* crashes on Windows */
        IsDialogMessageA(NULL, NULL);
        IsDialogMessageA(g_hwndMain, NULL);
    }

    /* The focus should initially be nowhere.  The first TAB should take it
     * to the first button.  The second TAB should take it to the Cancel
     * button.
     */

    /* valid window, invalid message window */
    hook = SetWindowsHookExA(WH_MSGFILTER, hook_proc2, NULL, GetCurrentThreadId());
    FormTabMsg (&msg, (HWND)0xbeefbeef);
    ok (!IsDialogMessageA(g_hwndMain, &msg), "expected failure\n");
    ok(g_MSGF_DIALOGBOX, "hook wasn't called\n");
    g_MSGF_DIALOGBOX = FALSE;
    UnhookWindowsHookEx(hook);

    hook = SetWindowsHookExA(WH_MSGFILTER, hook_proc, NULL, GetCurrentThreadId());
    FormTabMsg (&msg, g_hwndMain);

    ok (!IsDialogMessageA(NULL, &msg), "expected failure\n");
    ok (!IsDialogMessageA((HWND)0xbeefbeef, &msg), "expected failure\n");

    UnhookWindowsHookEx(hook);

    ok (IsDialogMessageA(g_hwndMain, &msg), "Did not handle first TAB\n");
    ok ((GetFocus() == g_hwndButton1), "Focus did not move to first button\n");
    FormTabMsg (&msg, g_hwndButton1);
    ok (IsDialogMessageA(g_hwndMain, &msg), "Did not handle second TAB\n");
    ok ((GetFocus() == g_hwndButtonCancel),
            "Focus did not move to cancel button\n");
    FormEnterMsg (&msg, g_hwndButtonCancel);
    ok (IsDialogMessageA(g_hwndMain, &msg), "Did not handle the ENTER\n");
    ok (g_terminated, "ENTER did not terminate\n");

    /* matching but invalid window handles, NULL */
    hook = SetWindowsHookExA(WH_MSGFILTER, hook_proc, NULL, GetCurrentThreadId());

    FormTabMsg (&msg, NULL);
    ok (!IsDialogMessageA(msg.hwnd, &msg), "expected failure\n");

    /* matching but invalid window handles, not NULL */
    FormTabMsg (&msg, (HWND)0xbeefbeef);
    ok (!IsDialogMessageA(msg.hwnd, &msg), "expected failure\n");

    UnhookWindowsHookEx(hook);
    DestroyWindow(g_hwndMain);

    g_hwndMain = CreateWindowA("IsDialogMessageWindowClass", "IsDialogMessageWindowClass", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, g_hinst, 0);
    SetFocus(g_hwndButton1);
    g_button1Clicked = FALSE;
    FormEnterMsg(&msg, g_hwndButton1);
    ok(IsDialogMessageA(g_hwndMain, &msg), "Did not handle the ENTER\n");
    ok(g_button1Clicked, "Did not receive button 1 click notification\n");

    g_button1Clicked = FALSE;
    FormEnterMsg(&msg, g_hwndMain);
    ok(IsDialogMessageA(g_hwndMain, &msg), "Did not handle the ENTER\n");
    ok(g_button1Clicked, "Did not receive button 1 click notification\n");

    g_button1Clicked = FALSE;
    FormEnterMsg(&msg, g_hwndButton2);
    ok(IsDialogMessageA(g_hwndMain, &msg), "Did not handle the ENTER\n");
    ok(g_button1Clicked, "Did not receive button 1 click notification\n");

    /* Button with id larger than 0xFFFF should also work */
    g_button1Clicked = FALSE;
    FormEnterMsg(&msg, g_hwndMain);
    SetWindowLongPtrW(g_hwndButton1, GWLP_ID, 0x1FFFF);
    ok(IsDialogMessageA(g_hwndMain, &msg), "Did not handle the ENTER\n");
    ok(g_button1Clicked, "Did not receive button 1 click notification\n");

    DestroyWindow(g_hwndMain);
}


static INT_PTR CALLBACK delayFocusDlgWinProc (HWND hDlg, UINT uiMsg, WPARAM wParam,
        LPARAM lParam)
{
    switch (uiMsg)
    {
    case WM_INITDIALOG:
        g_hwndMain = hDlg;
       g_hwndInitialFocusGroupBox = GetDlgItem(hDlg,100);
       g_hwndButton1 = GetDlgItem(hDlg,200);
       g_hwndButton2 = GetDlgItem(hDlg,201);
       g_hwndButtonCancel = GetDlgItem(hDlg,IDCANCEL);
       g_styleInitialFocusT1 = GetWindowLongA(g_hwndInitialFocusGroupBox, GWL_STYLE);

       /* Initially check the second radio button */
       SendMessageA(g_hwndButton1, BM_SETCHECK, BST_UNCHECKED, 0);
       SendMessageA(g_hwndButton2, BM_SETCHECK, BST_CHECKED  , 0);
       /* Continue testing after dialog initialization */
       PostMessageA(hDlg, WM_USER, 0, 0);
       return g_bInitialFocusInitDlgResult;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL)
       {
           EndDialog(hDlg, LOWORD(wParam));
           return TRUE;
       }
       return FALSE;

    case WM_USER:
       g_styleInitialFocusT2 = GetWindowLongA(hDlg, GWL_STYLE);
        g_hwndInitialFocusT1 = GetFocus();
       SetFocus(hDlg);
        g_hwndInitialFocusT2 = GetFocus();
       PostMessageA(hDlg, WM_COMMAND, IDCANCEL, 0);
       return TRUE;
    }

    return FALSE;
}

static INT_PTR CALLBACK focusDlgWinProc (HWND hDlg, UINT uiMsg, WPARAM wParam,
        LPARAM lParam)
{
    switch (uiMsg)
    {
    case WM_INITDIALOG:
        SetWindowTextA(GetDlgItem(hDlg, 200), "new caption");
        return TRUE;

    case WM_COMMAND:
       if (LOWORD(wParam) == 200)
       {
           if (HIWORD(wParam) == EN_SETFOCUS)
               g_hwndInitialFocusT1 = (HWND)lParam;
       }
       return FALSE;
    }

    return FALSE;
}

static INT_PTR CALLBACK EmptyProcUserTemplate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg) {
    case WM_INITDIALOG:
        return TRUE;
    }
    return FALSE;
}

static INT_PTR CALLBACK focusChildDlgWinProc (HWND hwnd, UINT uiMsg, WPARAM wParam,
        LPARAM lParam)
{
    static HWND hChildDlg;

    switch (uiMsg)
    {
    case WM_INITDIALOG:
    {
        RECT rectHwnd;
        struct  {
            DLGTEMPLATE tmplate;
            WORD menu,class,title;
        } temp;

        SetFocus( GetDlgItem(hwnd, 200) );

        GetClientRect(hwnd,&rectHwnd);
        temp.tmplate.style = WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | DS_CONTROL | DS_3DLOOK;
        temp.tmplate.dwExtendedStyle = 0;
        temp.tmplate.cdit = 0;
        temp.tmplate.x = 0;
        temp.tmplate.y = 0;
        temp.tmplate.cx = 0;
        temp.tmplate.cy = 0;
        temp.menu = temp.class = temp.title = 0;

        hChildDlg = CreateDialogIndirectParamA(g_hinst, &temp.tmplate,
                  hwnd, (DLGPROC)EmptyProcUserTemplate, 0);
        ok(hChildDlg != 0, "Failed to create test dialog.\n");

        return FALSE;
    }
    case WM_CLOSE:
        DestroyWindow(hChildDlg);
        return TRUE;
    }

    return FALSE;
}

/* Helper for InitialFocusTest */
static const char * GetHwndString(HWND hw)
{
  if (hw == NULL)
    return "a null handle";
  if (hw == g_hwndMain)
    return "the dialog handle";
  if (hw == g_hwndInitialFocusGroupBox)
    return "the group box control";
  if (hw == g_hwndButton1)
    return "the first button";
  if (hw == g_hwndButton2)
    return "the second button";
  if (hw == g_hwndButtonCancel)
    return "the cancel button";

  return "unknown handle";
}

static void test_focus(void)
{
    /* Test 1:
     * This test intentionally returns FALSE in response to WM_INITDIALOG
     * without setting focus to a control. This is what MFC's CFormView does.
     *
     * Since the WM_INITDIALOG handler returns FALSE without setting the focus,
     * the focus should initially be NULL. Later, when we manually set focus to
     * the dialog, the default handler should set focus to the first control that
     * is "visible, not disabled, and has the WS_TABSTOP style" (MSDN). Because the
     * second radio button has been checked, it should be the first control
     * that meets these criteria and should receive the focus.
     */

    g_bInitialFocusInitDlgResult = FALSE;
    g_hwndInitialFocusT1 = (HWND) -1;
    g_hwndInitialFocusT2 = (HWND) -1;
    g_styleInitialFocusT1 = -1;
    g_styleInitialFocusT2 = -1;

    DialogBoxA(g_hinst, "RADIO_TEST_DIALOG", NULL, delayFocusDlgWinProc);

    ok (((g_styleInitialFocusT1 & WS_TABSTOP) == 0),
       "Error in wrc - Detected WS_TABSTOP as default style for GROUPBOX\n");

    ok (((g_styleInitialFocusT2 & WS_VISIBLE) == 0),
       "Modal dialogs should not be shown until the message queue first goes empty\n");

    ok ((g_hwndInitialFocusT1 == NULL),
        "Error in initial focus when WM_INITDIALOG returned FALSE: "
        "Expected NULL focus, got %s (%p).\n",
        GetHwndString(g_hwndInitialFocusT1), g_hwndInitialFocusT1);

    ok ((g_hwndInitialFocusT2 == g_hwndButton2),
        "Error after first SetFocus() when WM_INITDIALOG returned FALSE: "
        "Expected the second button (%p), got %s (%p).\n",
        g_hwndButton2, GetHwndString(g_hwndInitialFocusT2),
        g_hwndInitialFocusT2);

    /* Test 2:
     * This is the same as above, except WM_INITDIALOG is made to return TRUE.
     * This should cause the focus to go to the second radio button right away
     * and stay there (until the user indicates otherwise).
     */

    g_bInitialFocusInitDlgResult = TRUE;
    g_hwndInitialFocusT1 = (HWND) -1;
    g_hwndInitialFocusT2 = (HWND) -1;
    g_styleInitialFocusT1 = -1;
    g_styleInitialFocusT2 = -1;

    DialogBoxA(g_hinst, "RADIO_TEST_DIALOG", NULL, delayFocusDlgWinProc);

    ok ((g_hwndInitialFocusT1 == g_hwndButton2),
       "Error in initial focus when WM_INITDIALOG returned TRUE: "
       "Expected the second button (%p), got %s (%p).\n",
       g_hwndButton2, GetHwndString(g_hwndInitialFocusT1),
       g_hwndInitialFocusT1);

    ok ((g_hwndInitialFocusT2 == g_hwndButton2),
       "Error after first SetFocus() when WM_INITDIALOG returned TRUE: "
       "Expected the second button (%p), got %s (%p).\n",
       g_hwndButton2, GetHwndString(g_hwndInitialFocusT2),
       g_hwndInitialFocusT2);

    /* Test 3:
     * If the dialog has DS_CONTROL and it's not visible then we shouldn't change focus */
    {
        HWND hDlg;
        HRSRC hResource;
        HANDLE hTemplate;
        DLGTEMPLATE* pTemplate;
        HWND hTextbox;
        DWORD selectionStart = 0xdead, selectionEnd = 0xbeef;

        hResource = FindResourceA(g_hinst,"FOCUS_TEST_DIALOG", (LPCSTR)RT_DIALOG);
        hTemplate = LoadResource(g_hinst, hResource);
        pTemplate = LockResource(hTemplate);

        g_hwndInitialFocusT1 = 0;
        hDlg = CreateDialogIndirectParamA(g_hinst, pTemplate, NULL, focusDlgWinProc, 0);
        ok (hDlg != 0, "Failed to create test dialog.\n");

        ok ((g_hwndInitialFocusT1 == 0),
            "Focus should not be set for an invisible DS_CONTROL dialog %p.\n", g_hwndInitialFocusT1);

        /* Also make sure that WM_SETFOCUS selects the textbox's text */
        hTextbox = GetDlgItem(hDlg, 200);
        SendMessageA(hTextbox, WM_SETTEXT, 0, (LPARAM)"Hello world");

        SendMessageA(hDlg, WM_SETFOCUS, 0, 0);
        SendMessageA(hTextbox, EM_GETSEL, (WPARAM)&selectionStart, (LPARAM)&selectionEnd);
        ok(selectionStart == 0 && selectionEnd == 11, "Text selection after WM_SETFOCUS is [%i, %i) expected [0, 11)\n", selectionStart, selectionEnd);

        /* but WM_ACTIVATE does not */
        SendMessageA(hTextbox, EM_SETSEL, 0, 0);
        SendMessageA(hDlg, WM_ACTIVATE, WA_ACTIVE, 0);
        SendMessageA(hTextbox, EM_GETSEL, (WPARAM)&selectionStart, (LPARAM)&selectionEnd);
        ok(selectionStart == 0 && selectionEnd == 0, "Text selection after WM_ACTIVATE is [%i, %i) expected [0, 0)\n", selectionStart, selectionEnd);

        DestroyWindow(hDlg);
    }

    /* Test 4:
     * If the dialog has no tab-accessible controls, set focus to first control */
    {
        HWND hDlg;
        HRSRC hResource;
        HANDLE hTemplate;
        DLGTEMPLATE* pTemplate;
        HWND hLabel;

        hResource = FindResourceA(g_hinst,"FOCUS_TEST_DIALOG_2", (LPCSTR)RT_DIALOG);
        hTemplate = LoadResource(g_hinst, hResource);
        pTemplate = LockResource(hTemplate);

        hDlg = CreateDialogIndirectParamA(g_hinst, pTemplate, NULL, focusDlgWinProc, 0);
        ok(hDlg != 0, "Failed to create test dialog.\n");
        hLabel = GetDlgItem(hDlg, 200);

        ok(GetFocus() == hLabel, "Focus not set to label, focus=%p dialog=%p label=%p\n", GetFocus(), hDlg, hLabel);

        DestroyWindow(hDlg);

        /* Also check focus after WM_ACTIVATE and WM_SETFOCUS */
        hDlg = CreateDialogIndirectParamA(g_hinst, pTemplate, NULL, NULL, 0);
        ok(hDlg != 0, "Failed to create test dialog.\n");
        hLabel = GetDlgItem(hDlg, 200);

        SetFocus(NULL);
        SendMessageA(hDlg, WM_ACTIVATE, WA_ACTIVE, 0);
        ok(GetFocus() == NULL, "Focus set on WM_ACTIVATE, focus=%p dialog=%p label=%p\n", GetFocus(), hDlg, hLabel);

        SetFocus(NULL);
        SendMessageA(hDlg, WM_SETFOCUS, 0, 0);
        ok(GetFocus() == hLabel, "Focus not set to label on WM_SETFOCUS, focus=%p dialog=%p label=%p\n", GetFocus(), hDlg, hLabel);

        DestroyWindow(hDlg);
    }

    /* Test 5:
     * Select textbox's text on creation */
    {
        HWND hDlg;
        HRSRC hResource;
        HANDLE hTemplate;
        DLGTEMPLATE* pTemplate;
        HWND edit;
        DWORD selectionStart = 0xdead, selectionEnd = 0xbeef;

        hResource = FindResourceA(g_hinst,"FOCUS_TEST_DIALOG_3", (LPCSTR)RT_DIALOG);
        hTemplate = LoadResource(g_hinst, hResource);
        pTemplate = LockResource(hTemplate);

        hDlg = CreateDialogIndirectParamA(g_hinst, pTemplate, NULL, focusDlgWinProc, 0);
        ok(hDlg != 0, "Failed to create test dialog.\n");
        edit = GetDlgItem(hDlg, 200);

        ok(GetFocus() == edit, "Focus not set to edit, focus=%p, dialog=%p, edit=%p\n",
                GetFocus(), hDlg, edit);
        SendMessageA(edit, EM_GETSEL, (WPARAM)&selectionStart, (LPARAM)&selectionEnd);
        ok(selectionStart == 0 && selectionEnd == 11,
                "Text selection after WM_SETFOCUS is [%i, %i) expected [0, 11)\n",
                selectionStart, selectionEnd);

        DestroyWindow(hDlg);
    }

    /* Test 6:
     * Select textbox's text on creation when WM_INITDIALOG creates a child dialog. */
    {
        HWND hDlg;
        HRSRC hResource;
        HANDLE hTemplate;
        DLGTEMPLATE* pTemplate;
        HWND edit;

        hResource = FindResourceA(g_hinst,"FOCUS_TEST_DIALOG_3", (LPCSTR)RT_DIALOG);
        hTemplate = LoadResource(g_hinst, hResource);
        pTemplate = LockResource(hTemplate);

        hDlg = CreateDialogIndirectParamA(g_hinst, pTemplate, NULL, focusChildDlgWinProc, 0);
        ok(hDlg != 0, "Failed to create test dialog.\n");
        edit = GetDlgItem(hDlg, 200);

        ok(GetFocus() == edit, "Focus not set to edit, focus=%p, dialog=%p, edit=%p\n",
                GetFocus(), hDlg, edit);

        DestroyWindow(hDlg);
    }
}

static void test_GetDlgItemText(void)
{
    char string[64];
    BOOL ret;

    strcpy(string, "Overwrite Me");
    ret = GetDlgItemTextA(NULL, 0, string, ARRAY_SIZE(string));
    ok(!ret, "GetDlgItemText(NULL) shouldn't have succeeded\n");

    ok(string[0] == '\0' || broken(!strcmp(string, "Overwrite Me")),
       "string retrieved using GetDlgItemText should have been NULL terminated\n");
}

static INT_PTR CALLBACK getdlgitem_test_dialog_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_INITDIALOG)
    {
        char text[64];
        LONG_PTR val;
        HWND hwnd;
        BOOL ret;

        hwnd = GetDlgItem(hdlg, -1);
        ok(hwnd != NULL, "Expected dialog item.\n");

        *text = 0;
        ret = GetDlgItemTextA(hdlg, -1, text, ARRAY_SIZE(text));
        ok(ret && !strcmp(text, "Text1"), "Unexpected item text.\n");

        val = GetWindowLongA(hwnd, GWLP_ID);
        ok(val == -1, "Unexpected id.\n");

        val = GetWindowLongPtrA(hwnd, GWLP_ID);
        ok(val == -1, "Unexpected id %ld.\n", val);

        hwnd = GetDlgItem(hdlg, -2);
        ok(hwnd != NULL, "Expected dialog item.\n");

        val = GetWindowLongA(hwnd, GWLP_ID);
        ok(val == -2, "Unexpected id.\n");

        val = GetWindowLongPtrA(hwnd, GWLP_ID);
        ok(val == -2, "Unexpected id %ld.\n", val);

        EndDialog(hdlg, 0xdead);
    }

    return FALSE;
}

static void test_GetDlgItem(void)
{
    HWND hwnd, child1, child2, hwnd2;
    INT_PTR retval;
    BOOL ret;

    hwnd = CreateWindowA("button", "parent", WS_VISIBLE, 0, 0, 100, 100, NULL, 0, g_hinst, NULL);
    ok(hwnd != NULL, "failed to created window\n");

    /* created with the same ID */
    child1 = CreateWindowA("button", "child1", WS_VISIBLE|WS_CHILD, 0, 0, 10, 10, hwnd, 0, g_hinst, NULL);
    ok(child1 != NULL, "failed to create first child\n");
    child2 = CreateWindowA("button", "child2", WS_VISIBLE|WS_CHILD, 0, 0, 10, 10, hwnd, 0, g_hinst, NULL);
    ok(child2 != NULL, "failed to create second child\n");

    hwnd2 = GetDlgItem(hwnd, 0);
    ok(hwnd2 == child1, "expected first child, got %p\n", hwnd2);

    hwnd2 = GetTopWindow(hwnd);
    ok(hwnd2 == child1, "expected first child to be top, got %p\n", hwnd2);

    ret = SetWindowPos(child1, child2, 0, 0, 0, 0, SWP_NOMOVE);
    ok(ret, "got %d\n", ret);
    hwnd2 = GetTopWindow(hwnd);
    ok(hwnd2 == child2, "expected second child to be top, got %p\n", hwnd2);

    /* top window from child list is picked */
    hwnd2 = GetDlgItem(hwnd, 0);
    ok(hwnd2 == child2, "expected second child, got %p\n", hwnd2);

    /* Now test how GetDlgItem searches */
    DestroyWindow(child2);
    child2 = CreateWindowA("button", "child2", WS_VISIBLE|WS_CHILD, 0, 0, 10, 10, child1, 0, g_hinst, NULL);
    ok(child2 != NULL, "failed to create second child\n");

    /* give child2 an ID */
    SetWindowLongA(child2, GWLP_ID, 100);

    hwnd2 = GetDlgItem(hwnd, 100);
    ok(!hwnd2, "expected child to not be found, got %p\n", hwnd2);

    /* make the ID of child2 public with a WS_EX_CONTROLPARENT parent */
    SetWindowLongA(child1, GWL_EXSTYLE, WS_EX_CONTROLPARENT);

    hwnd2 = GetDlgItem(hwnd, 100);
    ok(!hwnd2, "expected child to not be found, got %p\n", hwnd2);

    DestroyWindow(child1);
    DestroyWindow(child2);
    DestroyWindow(hwnd);

    retval = DialogBoxParamA(g_hinst, "GETDLGITEM_TEST_DIALOG", NULL, getdlgitem_test_dialog_proc, 0);
    ok(retval == 0xdead, "Unexpected return value.\n");
}

static INT_PTR CALLBACK DestroyDlgWinProc (HWND hDlg, UINT uiMsg,
        WPARAM wParam, LPARAM lParam)
{
    if (uiMsg == WM_INITDIALOG)
    {
        DestroyWindow(hDlg);
        return TRUE;
    }
    return FALSE;
}

static INT_PTR CALLBACK DestroyOnCloseDlgWinProc (HWND hDlg, UINT uiMsg,
        WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg)
    {
    case WM_INITDIALOG:
        PostMessageA(hDlg, WM_CLOSE, 0, 0);
        return TRUE;
    case WM_CLOSE:
        DestroyWindow(hDlg);
        return TRUE;
    case WM_DESTROY:
        PostQuitMessage(0);
        return TRUE;
    }
    return FALSE;
}

static INT_PTR CALLBACK TestInitDialogHandleProc (HWND hDlg, UINT uiMsg,
        WPARAM wParam, LPARAM lParam)
{
    if (uiMsg == WM_INITDIALOG)
    {
        HWND expected = GetNextDlgTabItem(hDlg, NULL, FALSE);
        ok(expected == (HWND)wParam,
           "Expected wParam to be the handle to the first tabstop control (%p), got %p\n",
           expected, (HWND)wParam);

        EndDialog(hDlg, LOWORD(SendMessageA(hDlg, DM_GETDEFID, 0, 0)));
        return TRUE;
    }
    return FALSE;
}

static INT_PTR CALLBACK TestDefButtonDlgProc (HWND hDlg, UINT uiMsg,
                                              WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg)
    {
    case WM_INITDIALOG:
        EndDialog(hDlg, LOWORD(SendMessageA(hDlg, DM_GETDEFID, 0, 0)));
        return TRUE;
    }
    return FALSE;
}

static INT_PTR CALLBACK TestReturnKeyDlgProc (HWND hDlg, UINT uiMsg,
        WPARAM wParam, LPARAM lParam)
{
    static int received_idok;

    switch (uiMsg)
    {
    case WM_INITDIALOG:
    {
        MSG msg = {hDlg, WM_KEYDOWN, VK_RETURN, 0x011c0001};

        received_idok = -1;
        IsDialogMessageA(hDlg, &msg);
        ok(received_idok == 0xdead, "WM_COMMAND/0xdead not received\n");

        received_idok = -2;
        IsDialogMessageA(hDlg, &msg);
        ok(received_idok == IDOK, "WM_COMMAND/IDOK not received\n");

        EndDialog(hDlg, 0);
        return TRUE;
    }

    case DM_GETDEFID:
        if (received_idok == -1)
        {
            HWND hwnd = GetDlgItem(hDlg, 0xdead);
            ok(!hwnd, "dialog item with ID 0xdead should not exist\n");
            SetWindowLongA(hDlg, DWLP_MSGRESULT, MAKELRESULT(0xdead, DC_HASDEFID));
            return TRUE;
        }
        return FALSE;

    case WM_COMMAND:
        received_idok = wParam;
        return TRUE;
    }
    return FALSE;
}

static INT_PTR CALLBACK TestControlStyleDlgProc(HWND hdlg, UINT msg,
                                                WPARAM wparam, LPARAM lparam)
{
    HWND control;
    DWORD style, exstyle;
    char buf[256];

    switch (msg)
    {
    case WM_INITDIALOG:
        control = GetDlgItem(hdlg, 7);
        ok(control != 0, "dialog control with id 7 not found\n");
        style = GetWindowLongA(control, GWL_STYLE);
        ok(style == (WS_CHILD|WS_VISIBLE), "expected WS_CHILD|WS_VISIBLE, got %#x\n", style);
        exstyle = GetWindowLongA(control, GWL_EXSTYLE);
        ok(exstyle == (WS_EX_NOPARENTNOTIFY|WS_EX_TRANSPARENT|WS_EX_CLIENTEDGE), "expected WS_EX_NOPARENTNOTIFY|WS_EX_TRANSPARENT|WS_EX_CLIENTEDGE, got %#x\n", exstyle);
        buf[0] = 0;
        GetWindowTextA(control, buf, sizeof(buf));
        ok(strcmp(buf, "bump7") == 0,  "expected bump7, got %s\n", buf);

        control = GetDlgItem(hdlg, 8);
        ok(control != 0, "dialog control with id 8 not found\n");
        style = GetWindowLongA(control, GWL_STYLE);
        ok(style == (WS_CHILD|WS_VISIBLE), "expected WS_CHILD|WS_VISIBLE, got %#x\n", style);
        exstyle = GetWindowLongA(control, GWL_EXSTYLE);
        ok(exstyle == (WS_EX_NOPARENTNOTIFY|WS_EX_TRANSPARENT), "expected WS_EX_NOPARENTNOTIFY|WS_EX_TRANSPARENT, got %#x\n", exstyle);
        buf[0] = 0;
        GetWindowTextA(control, buf, sizeof(buf));
        ok(strcmp(buf, "bump8") == 0,  "expected bump8, got %s\n", buf);

        EndDialog(hdlg, -7);
        return TRUE;
    }
    return FALSE;
}

static const WCHAR testtextW[] = {'W','n','d','T','e','x','t',0};
static const char *testtext = "WndText";

enum defdlgproc_text
{
    DLGPROCTEXT_SNDMSGA = 0,
    DLGPROCTEXT_SNDMSGW,
    DLGPROCTEXT_DLGPROCA,
    DLGPROCTEXT_DLGPROCW,
    DLGPROCTEXT_SETTEXTA,
    DLGPROCTEXT_SETTEXTW,
};

static const char *testmodes[] =
{
    "SNDMSGA",
    "SNDMSGW",
    "DLGPROCA",
    "DLGPROCW",
    "SETTEXTA",
    "SETTEXTW",
};

static INT_PTR CALLBACK test_aw_conversion_dlgprocA(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    int mode = HandleToULong(GetPropA(hdlg, "test_mode"));
    WCHAR *text = (WCHAR *)lparam;
    char *textA = (char *)lparam;

    switch (msg)
    {
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
        switch (mode)
        {
        case DLGPROCTEXT_DLGPROCA:
            ok(textA == testtext, "%s: %s, unexpected text %s.\n", IsWindowUnicode(hdlg) ? "U" : "A",
                testmodes[mode], textA);
            break;
        case DLGPROCTEXT_DLGPROCW:
            ok(text == testtextW, "%s: %s, unexpected text %s.\n", IsWindowUnicode(hdlg) ? "U" : "A", testmodes[mode],
                wine_dbgstr_w(text));
            break;
        case DLGPROCTEXT_SNDMSGA:
        case DLGPROCTEXT_SETTEXTA:
            if (IsWindowUnicode(hdlg))
            {
                ok(text != testtextW && !lstrcmpW(text, testtextW),
                    "U: %s, unexpected text %s.\n", testmodes[mode], wine_dbgstr_w(text));
            }
            else
                ok(textA == testtext, "A: %s, unexpected text %s.\n", testmodes[mode], textA);
            break;
        case DLGPROCTEXT_SNDMSGW:
        case DLGPROCTEXT_SETTEXTW:
            if (IsWindowUnicode(hdlg))
                ok(text == testtextW, "U: %s, unexpected text %s.\n", testmodes[mode], wine_dbgstr_w(text));
            else
                ok(textA != testtext && !strcmp(textA, testtext), "A: %s, unexpected text %s.\n",
                    testmodes[mode], textA);
            break;
        }
        break;
    };

    return DefWindowProcW(hdlg, msg, wparam, lparam);
}

static INT_PTR CALLBACK test_aw_conversion_dlgprocW(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    int mode = HandleToULong(GetPropA(hdlg, "test_mode"));
    WCHAR *text = (WCHAR *)lparam;
    char *textA = (char *)lparam;

    switch (msg)
    {
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
        switch (mode)
        {
        case DLGPROCTEXT_DLGPROCA:
            ok(textA == testtext, "%s: %s, unexpected text %s.\n", IsWindowUnicode(hdlg) ? "U" : "A",
                testmodes[mode], textA);
            break;
        case DLGPROCTEXT_DLGPROCW:
            ok(text == testtextW, "%s: %s, unexpected text %s.\n", IsWindowUnicode(hdlg) ? "U" : "A", testmodes[mode],
                wine_dbgstr_w(text));
            break;
        case DLGPROCTEXT_SNDMSGA:
        case DLGPROCTEXT_SETTEXTA:
            if (IsWindowUnicode(hdlg))
                ok(text != testtextW && !lstrcmpW(text, testtextW),
                    "U: %s, unexpected text %s.\n", testmodes[mode], wine_dbgstr_w(text));
            else
                ok(textA == testtext, "A: %s, unexpected text %s.\n", testmodes[mode], textA);
            break;
        case DLGPROCTEXT_SNDMSGW:
        case DLGPROCTEXT_SETTEXTW:
            if (IsWindowUnicode(hdlg))
                ok(text == testtextW, "U: %s, unexpected text %s.\n", testmodes[mode], wine_dbgstr_w(text));
            else
                ok(textA != testtext && !strcmp(textA, testtext), "A: %s, unexpected text %s.\n",
                    testmodes[mode], textA);
            break;
        }
        break;
    }

    return DefWindowProcA(hdlg, msg, wparam, lparam);
}

static void dlg_test_aw_message(HWND hdlg, UINT msg)
{
    SetPropA(hdlg, "test_mode", ULongToHandle(DLGPROCTEXT_SNDMSGA));
    SendMessageA(hdlg, msg, 0, (LPARAM)testtext);

    SetPropA(hdlg, "test_mode", ULongToHandle(DLGPROCTEXT_SNDMSGW));
    SendMessageW(hdlg, msg, 0, (LPARAM)testtextW);

    SetPropA(hdlg, "test_mode", ULongToHandle(DLGPROCTEXT_DLGPROCA));
    DefDlgProcA(hdlg, msg, 0, (LPARAM)testtext);

    SetPropA(hdlg, "test_mode", ULongToHandle(DLGPROCTEXT_DLGPROCW));
    DefDlgProcW(hdlg, msg, 0, (LPARAM)testtextW);
}

static INT_PTR CALLBACK test_aw_conversion_dlgproc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    ULONG_PTR dlgproc, originalproc;
    WCHAR buffW[64];
    char buff[64];
    BOOL ret;
    INT len;

    switch (msg)
    {
    case WM_INITDIALOG:
        ok(IsWindowUnicode(hdlg), "Expected unicode window.\n");

        dlg_test_aw_message(hdlg, WM_WININICHANGE);
        dlg_test_aw_message(hdlg, WM_DEVMODECHANGE);
        dlg_test_aw_message(hdlg, CB_DIR);
        dlg_test_aw_message(hdlg, LB_DIR);
        dlg_test_aw_message(hdlg, LB_ADDFILE);
        dlg_test_aw_message(hdlg, EM_REPLACESEL);
        dlg_test_aw_message(hdlg, WM_SETTEXT);

        /* WM_SETTEXT/WM_GETTEXT */
        originalproc = GetWindowLongPtrW(hdlg, DWLP_DLGPROC);
        ok(originalproc == (ULONG_PTR)test_aw_conversion_dlgproc, "Unexpected dlg proc %#lx.\n", originalproc);

        dlgproc = GetWindowLongPtrA(hdlg, DWLP_DLGPROC);
        ok(dlgproc != (ULONG_PTR)test_aw_conversion_dlgproc, "Unexpected dlg proc %#lx.\n", dlgproc);

        dlgproc = SetWindowLongPtrA(hdlg, DWLP_DLGPROC, (UINT_PTR)test_aw_conversion_dlgprocA);
        ok(IsWindowUnicode(hdlg), "Expected unicode window.\n");

        dlgproc = GetWindowLongPtrW(hdlg, DWLP_DLGPROC);
        ok(dlgproc != (ULONG_PTR)test_aw_conversion_dlgprocA, "Unexpected dlg proc %#lx.\n", dlgproc);

        dlgproc = GetWindowLongPtrA(hdlg, DWLP_DLGPROC);
        ok(dlgproc == (ULONG_PTR)test_aw_conversion_dlgprocA, "Unexpected dlg proc %#lx.\n", dlgproc);

        SetPropA(hdlg, "test_mode", ULongToHandle(DLGPROCTEXT_SETTEXTA));
        ret = SetWindowTextA(hdlg, testtext);
    todo_wine
        ok(ret, "Failed to set window text.\n");

        SetPropA(hdlg, "test_mode", ULongToHandle(DLGPROCTEXT_SETTEXTW));
        ret = SetWindowTextW(hdlg, testtextW);
    todo_wine
        ok(ret, "Failed to set window text.\n");

        memset(buff, 'A', sizeof(buff));
        len = GetWindowTextA(hdlg, buff, sizeof(buff));
        ok(buff[0] == 0 && buff[1] == 'A' && len == 0, "Unexpected window text %#x, %#x, len %d\n",
           (BYTE)buff[0], (BYTE)buff[1], len);

        memset(buffW, 0xff, sizeof(buffW));
        len = GetWindowTextW(hdlg, buffW, 64);
        ok(!lstrcmpW(buffW, testtextW) && len == 0, "Unexpected window text %s, len %d\n", wine_dbgstr_w(buffW), len);

        dlg_test_aw_message(hdlg, WM_WININICHANGE);
        dlg_test_aw_message(hdlg, WM_DEVMODECHANGE);
        dlg_test_aw_message(hdlg, CB_DIR);
        dlg_test_aw_message(hdlg, LB_DIR);
        dlg_test_aw_message(hdlg, LB_ADDFILE);
        dlg_test_aw_message(hdlg, EM_REPLACESEL);
        dlg_test_aw_message(hdlg, WM_SETTEXT);

        dlgproc = SetWindowLongPtrW(hdlg, DWLP_DLGPROC, (UINT_PTR)test_aw_conversion_dlgprocW);
        ok(IsWindowUnicode(hdlg), "Expected unicode window.\n");

        dlgproc = GetWindowLongPtrW(hdlg, DWLP_DLGPROC);
        ok(dlgproc == (ULONG_PTR)test_aw_conversion_dlgprocW, "Unexpected dlg proc %#lx.\n", dlgproc);

        dlgproc = GetWindowLongPtrA(hdlg, DWLP_DLGPROC);
        ok(dlgproc != (ULONG_PTR)test_aw_conversion_dlgprocW, "Unexpected dlg proc %#lx.\n", dlgproc);

        SetPropA(hdlg, "test_mode", ULongToHandle(DLGPROCTEXT_SETTEXTA));
        ret = SetWindowTextA(hdlg, testtext);
    todo_wine
        ok(ret, "Failed to set window text.\n");

        SetPropA(hdlg, "test_mode", ULongToHandle(DLGPROCTEXT_SETTEXTW));
        ret = SetWindowTextW(hdlg, testtextW);
    todo_wine
        ok(ret, "Failed to set window text.\n");

        memset(buff, 'A', sizeof(buff));
        len = GetWindowTextA(hdlg, buff, sizeof(buff));
        ok(buff[0] == 0 && buff[1] == 'A' && len == 0, "Unexpected window text %#x, %#x, len %d\n",
           (BYTE)buff[0], (BYTE)buff[1], len);

        memset(buffW, 0xff, sizeof(buffW));
        len = GetWindowTextW(hdlg, buffW, ARRAY_SIZE(buffW));
        ok(buffW[0] == 'W' && buffW[1] == 0xffff && len == 0, "Unexpected window text %#x, %#x, len %d\n",
            buffW[0], buffW[1], len);

        dlg_test_aw_message(hdlg, WM_WININICHANGE);
        dlg_test_aw_message(hdlg, WM_DEVMODECHANGE);
        dlg_test_aw_message(hdlg, CB_DIR);
        dlg_test_aw_message(hdlg, LB_DIR);
        dlg_test_aw_message(hdlg, LB_ADDFILE);
        dlg_test_aw_message(hdlg, EM_REPLACESEL);
        dlg_test_aw_message(hdlg, WM_SETTEXT);

        SetWindowLongPtrA(hdlg, DWLP_DLGPROC, originalproc);
        EndDialog(hdlg, -123);
        return TRUE;
    }
    return FALSE;
}

static INT_PTR CALLBACK test_aw_conversion_dlgproc2(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    ULONG_PTR dlgproc, originalproc;
    WCHAR buffW[64];
    char buff[64];
    BOOL ret;
    INT len;

    switch (msg)
    {
    case WM_INITDIALOG:
        ok(!IsWindowUnicode(hdlg), "Unexpected unicode window.\n");

        dlg_test_aw_message(hdlg, WM_WININICHANGE);
        dlg_test_aw_message(hdlg, WM_DEVMODECHANGE);
        dlg_test_aw_message(hdlg, CB_DIR);
        dlg_test_aw_message(hdlg, LB_DIR);
        dlg_test_aw_message(hdlg, LB_ADDFILE);
        dlg_test_aw_message(hdlg, EM_REPLACESEL);
        dlg_test_aw_message(hdlg, WM_SETTEXT);

        originalproc = GetWindowLongPtrW(hdlg, DWLP_DLGPROC);
        ok(originalproc != (ULONG_PTR)test_aw_conversion_dlgproc2, "Unexpected dlg proc %#lx.\n", originalproc);

        dlgproc = GetWindowLongPtrA(hdlg, DWLP_DLGPROC);
        ok(dlgproc == (ULONG_PTR)test_aw_conversion_dlgproc2, "Unexpected dlg proc %#lx.\n", dlgproc);

        dlgproc = SetWindowLongPtrA(hdlg, DWLP_DLGPROC, (UINT_PTR)test_aw_conversion_dlgprocW);
        ok(!IsWindowUnicode(hdlg), "Unexpected unicode window.\n");

        dlgproc = GetWindowLongPtrW(hdlg, DWLP_DLGPROC);
        ok(dlgproc != (ULONG_PTR)test_aw_conversion_dlgprocW, "Unexpected dlg proc %#lx.\n", dlgproc);

        dlgproc = GetWindowLongPtrA(hdlg, DWLP_DLGPROC);
        ok(dlgproc == (ULONG_PTR)test_aw_conversion_dlgprocW, "Unexpected dlg proc %#lx.\n", dlgproc);

        SetPropA(hdlg, "test_mode", ULongToHandle(DLGPROCTEXT_SETTEXTA));
        ret = SetWindowTextA(hdlg, testtext);
    todo_wine
        ok(ret, "Failed to set window text.\n");

        SetPropA(hdlg, "test_mode", ULongToHandle(DLGPROCTEXT_SETTEXTW));
        ret = SetWindowTextW(hdlg, testtextW);
    todo_wine
        ok(ret, "Failed to set window text.\n");

        memset(buff, 'A', sizeof(buff));
        len = GetWindowTextA(hdlg, buff, sizeof(buff));
        ok(!strcmp(buff, testtext) && len == 0, "Unexpected window text %s, len %d\n", buff, len);

        memset(buffW, 0xff, sizeof(buffW));
        len = GetWindowTextW(hdlg, buffW, 64);
        ok(buffW[0] == 0 && buffW[1] == 0xffff && len == 0, "Unexpected window text %s, len %d\n",
            wine_dbgstr_w(buffW), len);

        dlg_test_aw_message(hdlg, WM_WININICHANGE);
        dlg_test_aw_message(hdlg, WM_DEVMODECHANGE);
        dlg_test_aw_message(hdlg, CB_DIR);
        dlg_test_aw_message(hdlg, LB_DIR);
        dlg_test_aw_message(hdlg, LB_ADDFILE);
        dlg_test_aw_message(hdlg, EM_REPLACESEL);
        dlg_test_aw_message(hdlg, WM_SETTEXT);

        dlgproc = SetWindowLongPtrW(hdlg, DWLP_DLGPROC, (UINT_PTR)test_aw_conversion_dlgprocA);
        ok(!IsWindowUnicode(hdlg), "Unexpected unicode window.\n");

        dlgproc = GetWindowLongPtrW(hdlg, DWLP_DLGPROC);
        ok(dlgproc == (ULONG_PTR)test_aw_conversion_dlgprocA, "Unexpected dlg proc %#lx.\n", dlgproc);

        dlgproc = GetWindowLongPtrA(hdlg, DWLP_DLGPROC);
        ok(dlgproc != (ULONG_PTR)test_aw_conversion_dlgprocA, "Unexpected dlg proc %#lx.\n", dlgproc);

        SetPropA(hdlg, "test_mode", ULongToHandle(DLGPROCTEXT_SETTEXTA));
        ret = SetWindowTextA(hdlg, testtext);
    todo_wine
        ok(ret, "Failed to set window text.\n");

        SetPropA(hdlg, "test_mode", ULongToHandle(DLGPROCTEXT_SETTEXTW));
        ret = SetWindowTextW(hdlg, testtextW);
    todo_wine
        ok(ret, "Failed to set window text.\n");

        memset(buff, 'A', sizeof(buff));
        len = GetWindowTextA(hdlg, buff, sizeof(buff));
        ok(!strcmp(buff, testtext) && len == 0, "Unexpected window text %s, len %d\n", buff, len);

        memset(buffW, 0xff, sizeof(buffW));
        len = GetWindowTextW(hdlg, buffW, ARRAY_SIZE(buffW));
        ok(buffW[0] == 0 && buffW[1] == 0xffff && len == 0, "Unexpected window text %#x, %#x, len %d\n",
            buffW[0], buffW[1], len);

        dlg_test_aw_message(hdlg, WM_WININICHANGE);
        dlg_test_aw_message(hdlg, WM_DEVMODECHANGE);
        dlg_test_aw_message(hdlg, CB_DIR);
        dlg_test_aw_message(hdlg, LB_DIR);
        dlg_test_aw_message(hdlg, LB_ADDFILE);
        dlg_test_aw_message(hdlg, EM_REPLACESEL);
        dlg_test_aw_message(hdlg, WM_SETTEXT);

        SetWindowLongPtrA(hdlg, DWLP_DLGPROC, originalproc);
        EndDialog(hdlg, -123);
        return TRUE;
    }
    return FALSE;
}

static LRESULT CALLBACK test_aw_conversion_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    int mode = HandleToULong(GetPropA(hwnd, "test_mode"));
    WCHAR *text = (WCHAR *)lparam;
    char *textA = (char *)lparam;

    switch (msg)
    {
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
        switch (mode)
        {
        case DLGPROCTEXT_SNDMSGA:
            if (IsWindowUnicode(hwnd))
                ok(text != testtextW && !lstrcmpW(text, testtextW),
                    "U: %s, unexpected text %s.\n", testmodes[mode], wine_dbgstr_w(text));
            else
                ok(textA == testtext, "A: %s, unexpected text %s.\n", testmodes[mode], textA);
            break;
        case DLGPROCTEXT_SNDMSGW:
            if (IsWindowUnicode(hwnd))
                ok(text == testtextW, "U: %s, unexpected text %s.\n", testmodes[mode], wine_dbgstr_w(text));
            else
                ok(textA != testtext && !strcmp(textA, testtext), "A: %s, unexpected text %s.\n",
                    testmodes[mode], textA);
            break;
        default:
            ok(0, "Unexpected test mode %d.\n", mode);
        }
        break;
    }

    return IsWindowUnicode(hwnd) ? CallWindowProcW(oldproc, hwnd, msg, wparam, lparam) :
        CallWindowProcA(oldproc, hwnd, msg, wparam, lparam);
}

static INT_PTR CALLBACK test_aw_conversion_dlgproc3(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    BOOL is_unicode = !!lparam;
    LONG_PTR oldproc;

    switch (msg)
    {
    case WM_INITDIALOG:
        ok(is_unicode == IsWindowUnicode(hdlg), "Unexpected unicode window property.\n");

        oldproc = SetWindowLongPtrA(hdlg, GWLP_WNDPROC, (LONG_PTR)test_aw_conversion_wndproc);
        SetWindowLongPtrA(hdlg, GWLP_USERDATA, oldproc);
        ok(!IsWindowUnicode(hdlg), "Unexpected unicode window.\n");

        dlg_test_aw_message(hdlg, WM_WININICHANGE);
        dlg_test_aw_message(hdlg, WM_DEVMODECHANGE);
        dlg_test_aw_message(hdlg, CB_DIR);
        dlg_test_aw_message(hdlg, LB_DIR);
        dlg_test_aw_message(hdlg, LB_ADDFILE);
        dlg_test_aw_message(hdlg, EM_REPLACESEL);
        dlg_test_aw_message(hdlg, WM_SETTEXT);

        SetWindowLongPtrW(hdlg, GWLP_WNDPROC, (LONG_PTR)test_aw_conversion_wndproc);
        ok(IsWindowUnicode(hdlg), "Expected unicode window.\n");

        dlg_test_aw_message(hdlg, WM_WININICHANGE);
        dlg_test_aw_message(hdlg, WM_DEVMODECHANGE);
        dlg_test_aw_message(hdlg, CB_DIR);
        dlg_test_aw_message(hdlg, LB_DIR);
        dlg_test_aw_message(hdlg, LB_ADDFILE);
        dlg_test_aw_message(hdlg, EM_REPLACESEL);
        dlg_test_aw_message(hdlg, WM_SETTEXT);

        SetWindowLongPtrA(hdlg, GWLP_WNDPROC, oldproc);
        EndDialog(hdlg, -123);
        return TRUE;
    }
    return FALSE;
}

static void test_DialogBoxParam(void)
{
    static const WCHAR nameW[] = {'T','E','S','T','_','E','M','P','T','Y','_','D','I','A','L','O','G',0};
    INT_PTR ret;
    HWND hwnd_invalid = (HWND)0x4444;

    ret = DialogBoxParamA(GetModuleHandleA(NULL), "TEST_DLG_CHILD_POPUP", 0, TestControlStyleDlgProc, 0);
    ok(ret == -7, "expected -7, got %ld\n", ret);

    SetLastError(0xdeadbeef);
    ret = DialogBoxParamA(GetModuleHandleA(NULL), "IDD_DIALOG" , hwnd_invalid, 0 , 0);
    ok(0 == ret || broken(ret == -1), "DialogBoxParamA returned %ld, expected 0\n", ret);
    ok(ERROR_INVALID_WINDOW_HANDLE == GetLastError() ||
       broken(GetLastError() == 0xdeadbeef),
       "got %d, expected ERROR_INVALID_WINDOW_HANDLE\n",GetLastError());

    /* Test a dialog which destroys itself on WM_INITDIALOG. */
    SetLastError(0xdeadbeef);
    ret = DialogBoxParamA(GetModuleHandleA(NULL), "IDD_DIALOG", 0, DestroyDlgWinProc, 0);
    ok(-1 == ret, "DialogBoxParamA returned %ld, expected -1\n", ret);
    ok(ERROR_INVALID_WINDOW_HANDLE == GetLastError() ||
       GetLastError() == ERROR_SUCCESS ||
       broken(GetLastError() == 0xdeadbeef),
       "got %d, expected ERROR_INVALID_WINDOW_HANDLE\n",GetLastError());

    /* Test a dialog which destroys itself on WM_CLOSE. */
    ret = DialogBoxParamA(GetModuleHandleA(NULL), "IDD_DIALOG", 0, DestroyOnCloseDlgWinProc, 0);
    ok(0 == ret, "DialogBoxParamA returned %ld, expected 0\n", ret);

    SetLastError(0xdeadbeef);
    ret = DialogBoxParamA(GetModuleHandleA(NULL), "RESOURCE_INVALID" , 0, 0, 0);
    ok(-1 == ret, "DialogBoxParamA returned %ld, expected -1\n", ret);
    ok(ERROR_RESOURCE_NAME_NOT_FOUND == GetLastError() ||
       broken(GetLastError() == 0xdeadbeef),
       "got %d, expected ERROR_RESOURCE_NAME_NOT_FOUND\n",GetLastError());

    SetLastError(0xdeadbeef);
    ret = DialogBoxParamA(GetModuleHandleA(NULL), "TEST_DIALOG_INVALID_CLASS", 0, DestroyDlgWinProc, 0);
    ok(ret == -1, "DialogBoxParamA returned %ld, expected -1\n", ret);
    ok(GetLastError() == 0, "got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DefDlgProcA(0, WM_ERASEBKGND, 0, 0);
    ok(ret == 0, "DefDlgProcA returned %ld, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE ||
       broken(GetLastError() == 0xdeadbeef),
       "got %d, expected ERROR_INVALID_WINDOW_HANDLE\n", GetLastError());

    ret = DialogBoxParamA(GetModuleHandleA(NULL), "TEST_EMPTY_DIALOG", 0, TestInitDialogHandleProc, 0);
    ok(ret == IDOK, "Expected IDOK\n");

    ret = DialogBoxParamA(GetModuleHandleA(NULL), "TEST_EMPTY_DIALOG", 0, TestDefButtonDlgProc, 0);
    ok(ret == IDOK, "Expected IDOK\n");

    ret = DialogBoxParamA(GetModuleHandleA(NULL), "TEST_EMPTY_DIALOG", 0, TestReturnKeyDlgProc, 0);
    ok(ret == 0, "Unexpected ret value %ld.\n", ret);

    /* WM_SETTEXT handling in case of A/W dialog procedures vs A/W dialog window.  */
    ret = DialogBoxParamW(GetModuleHandleA(NULL), nameW, 0, test_aw_conversion_dlgproc, 0);
    ok(ret == -123, "Unexpected ret value %ld.\n", ret);

    ret = DialogBoxParamA(GetModuleHandleA(NULL), "TEST_EMPTY_DIALOG", 0, test_aw_conversion_dlgproc2, 0);
    ok(ret == -123, "Unexpected ret value %ld.\n", ret);

    ret = DialogBoxParamW(GetModuleHandleA(NULL), nameW, 0, test_aw_conversion_dlgproc3, 1);
    ok(ret == -123, "Unexpected ret value %ld.\n", ret);

    ret = DialogBoxParamA(GetModuleHandleA(NULL), "TEST_EMPTY_DIALOG", 0, test_aw_conversion_dlgproc3, 0);
    ok(ret == -123, "Unexpected ret value %ld.\n", ret);
}

static void test_DisabledDialogTest(void)
{
    g_terminated = FALSE;
    DialogBoxParamA(g_hinst, "IDD_DIALOG", NULL, disabled_test_proc, 0);
    ok(FALSE == g_terminated, "dialog with disabled ok button has been terminated\n");
}

static INT_PTR CALLBACK messageBoxFontDlgWinProc (HWND hDlg, UINT uiMsg, WPARAM wParam,
        LPARAM lParam)
{
    if (uiMsg == WM_INITDIALOG) {
        SetFocus(hDlg);
        return 1;
    }

    return 0;
}

static void test_MessageBoxFontTest(void)
{
    /* This dialog template defines a dialog template which got 0x7fff as its
     * font size and omits the other font members. On WinNT, passing such a
     * dialog template to CreateDialogIndirectParamW will result in a dialog
     * being created which uses the message box font. We test that here.
     */

    static unsigned char dlgTemplate[] =
    {
        /* Dialog header */
        0x01,0x00,              /* Version */
        0xff,0xff,              /* Extended template marker */
        0x00,0x00,0x00,0x00,    /* Context Help ID */
        0x00,0x00,0x00,0x00,    /* Extended style */
        0xc0,0x00,0xc8,0x80,    /* Style (WS_SYSMENU|WS_CAPTION|WS_POPUP|DS_SETFONT|DS_MODALFRAME) */
        0x01,0x00,              /* Control count */
        0x00,0x00,              /* X */
        0x00,0x00,              /* Y */
        0x80,0x00,              /* Width */
        0x80,0x00,              /* Height */
        0x00,0x00,              /* Menu name */
        0x00,0x00,              /* Class name */
        'T',0x00,'e',0x00,      /* Caption (unicode) */
        's',0x00,'t',0x00,
        0x00,0x00,
        0xff,0x7f,              /* Font height (0x7fff = message box font) */

        /* Control #1 */
        0x00,0x00,              /* Align to DWORD (header is 42 bytes) */
        0x00,0x00,0x00,0x00,    /* Context Help ID */
        0x00,0x00,0x00,0x00,    /* Extended style */
        0x00,0x00,0x00,0x50,    /* Style (WS_CHILD|WS_VISIBLE) */
        0x00,0x00,              /* X */
        0x00,0x00,              /* Y */
        0x80,0x00,              /* Width */
        0x80,0x00,              /* Height */
        0x00,0x01,0x00,0x00,    /* Control ID (256) */
        0xff,0xff,0x82,0x00,    /* Class (Static) */
        'W',0x00,'I',0x00,      /* Caption (unicode) */
        'N',0x00,'E',0x00,
        ' ',0x00,'d',0x00,
        'i',0x00,'a',0x00,
        'l',0x00,'o',0x00,
        'g',0x00,' ',0x00,
        't',0x00,'e',0x00,
        's',0x00,'t',0x00,
        '.',0x00,0x00,0x00,
        0x00,0x00,              /* Size of extended data */

        0x00,0x00               /* Align to DWORD */
    };

    HWND hDlg;
    HFONT hFont;
    LOGFONTW lfStaticFont;
    NONCLIENTMETRICSW ncMetrics;

    /* Check if the dialog can be created from the template. On Win9x, this should fail
     * because we are calling the W function which is not implemented, but that's what
     * we want, because passing such a template to CreateDialogIndirectParamA would crash
     * anyway.
     */
    hDlg = CreateDialogIndirectParamW(g_hinst, (LPCDLGTEMPLATEW)dlgTemplate, NULL, messageBoxFontDlgWinProc, 0);
    if (!hDlg)
    {
        win_skip("dialog wasn't created\n");
        return;
    }

    hFont = (HFONT) SendDlgItemMessageW(hDlg, 256, WM_GETFONT, 0, 0);
    if (!hFont)
    {
        skip("dialog uses system font\n");
        DestroyWindow(hDlg);
        return;
    }
    GetObjectW(hFont, sizeof(LOGFONTW), &lfStaticFont);

    ncMetrics.cbSize = FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0, &ncMetrics, 0);
    ok( !memcmp(&lfStaticFont, &ncMetrics.lfMessageFont, FIELD_OFFSET(LOGFONTW, lfFaceName)) &&
        !lstrcmpW(lfStaticFont.lfFaceName, ncMetrics.lfMessageFont.lfFaceName),
        "dialog doesn't use message box font\n");
    DestroyWindow(hDlg);
}

static const char msgbox_title[] = "%5!z9ZXw*ia;57n/FGl.bCH,Su\"mfKN;foCqAU\'j6AmoJgAc_D:Z0A\'E6PF_O/w";
static WCHAR expectedOK[] =
{
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'%','5','!','z','9','Z','X','w','*','i','a',';','5','7','n','/','F','G','l','.','b','C','H',',','S','u','"','m','f',
'K','N',';','f','o','C','q','A','U','\'','j','6','A','m','o','J','g','A','c','_','D',':','Z','0','A','\'','E','6','P',
'F','_','O','/','w','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'M','e','s','s','a','g','e','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'O','K',' ',' ',' ','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n', 0
};
static WCHAR expectedOkCancel[] =
{
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'%','5','!','z','9','Z','X','w','*','i','a',';','5','7','n','/','F','G','l','.','b','C','H',',','S','u','"','m','f',
'K','N',';','f','o','C','q','A','U','\'','j','6','A','m','o','J','g','A','c','_','D',':','Z','0','A','\'','E','6','P',
'F','_','O','/','w','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'M','e','s','s','a','g','e','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'O','K',' ',' ',' ','C','a','n','c','e','l',' ',' ',' ','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n', 0
};
static WCHAR expectedAbortRetryIgnore[] =
{
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'%','5','!','z','9','Z','X','w','*','i','a',';','5','7','n','/','F','G','l','.','b','C','H',',','S','u','"','m','f',
'K','N',';','f','o','C','q','A','U','\'','j','6','A','m','o','J','g','A','c','_','D',':','Z','0','A','\'','E','6','P',
'F','_','O','/','w','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'M','e','s','s','a','g','e','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'A','b','o','r','t',' ',' ',' ','R','e','t','r','y',' ',' ',' ','I','g','n','o','r','e',' ',' ',' ','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n', 0
};

static WCHAR expectedYesNo[] =
{
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'%','5','!','z','9','Z','X','w','*','i','a',';','5','7','n','/','F','G','l','.','b','C','H',',','S','u','"','m','f',
'K','N',';','f','o','C','q','A','U','\'','j','6','A','m','o','J','g','A','c','_','D',':','Z','0','A','\'','E','6','P',
'F','_','O','/','w','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'M','e','s','s','a','g','e','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'Y','e','s',' ',' ',' ','N','o',' ',' ',' ','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n', 0
};
static WCHAR expectedYesNoCancel[] =
{
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'%','5','!','z','9','Z','X','w','*','i','a',';','5','7','n','/','F','G','l','.','b','C','H',',','S','u','"','m','f',
'K','N',';','f','o','C','q','A','U','\'','j','6','A','m','o','J','g','A','c','_','D',':','Z','0','A','\'','E','6','P',
'F','_','O','/','w','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'M','e','s','s','a','g','e','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'Y','e','s',' ',' ',' ','N','o',' ',' ',' ','C','a','n','c','e','l',' ',' ',' ','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n', 0
};
static WCHAR expectedRetryCancel[] =
{
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'%','5','!','z','9','Z','X','w','*','i','a',';','5','7','n','/','F','G','l','.','b','C','H',',','S','u','"','m','f',
'K','N',';','f','o','C','q','A','U','\'','j','6','A','m','o','J','g','A','c','_','D',':','Z','0','A','\'','E','6','P',
'F','_','O','/','w','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'M','e','s','s','a','g','e','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'R','e','t','r','y',' ',' ',' ','C','a','n','c','e','l',' ',' ',' ','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n', 0
};
static WCHAR expectedCancelTryContinue[] =
{
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'%','5','!','z','9','Z','X','w','*','i','a',';','5','7','n','/','F','G','l','.','b','C','H',',','S','u','"','m','f',
'K','N',';','f','o','C','q','A','U','\'','j','6','A','m','o','J','g','A','c','_','D',':','Z','0','A','\'','E','6','P',
'F','_','O','/','w','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'M','e','s','s','a','g','e','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n',
'C','a','n','c','e','l',' ',' ',' ','T','r','y',' ','A','g','a','i','n',' ',' ',' ','C','o','n','t','i','n','u','e',' ',' ',' ','\r','\n',
'-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','\r','\n', 0
};

BOOL non_english = FALSE;

DWORD WINAPI WorkerThread(void *param)
{
    WCHAR *expected = param;
    char windowTitle[sizeof(msgbox_title)];
    HWND hwndMbox;
    BOOL succeeded = FALSE;

    Sleep(200);

    hwndMbox = GetForegroundWindow();

    /* Find the Window, if it doesn't have focus */
    if (!(IsWindow(hwndMbox) &&
        GetWindowTextA(hwndMbox, windowTitle, sizeof(msgbox_title)) &&
        lstrcmpA(msgbox_title, windowTitle) == 0))
    {
        hwndMbox = FindWindowA(NULL, msgbox_title);

        if (!IsWindow(hwndMbox))
            goto cleanup;
    }

    SendMessageA(hwndMbox, WM_COPY, 0, 0);

    if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(NULL))
    {
        HANDLE textHandle = GetClipboardData(CF_UNICODETEXT);
        WCHAR *text = GlobalLock(textHandle);

        if (text != NULL)
        {
            if(non_english)
                ok(lstrlenW(text) > 0, "Empty string on clipboard\n");
            else
            {
                succeeded = lstrcmpW(expected, text) == 0;
                if(!succeeded)
                {
                    ok(0, "%s\n", wine_dbgstr_w(text));
                    ok(0, "%s\n", wine_dbgstr_w(expected));
                }
            }

            GlobalUnlock(textHandle);
        }
        else
            ok(0, "No text on clipboard.\n");

        CloseClipboard();

    }
    else
        trace("Clipboard error\n");

    PostMessageA(hwndMbox, WM_COMMAND, IDIGNORE, 0); /* For MB_ABORTRETRYIGNORE dialog. */
    PostMessageA(hwndMbox, WM_CLOSE, 0, 0);

cleanup:
    ok(succeeded || non_english, "Failed to get string.\n");

    return 0;
}

static void test_MessageBox_WM_COPY_Test(void)
{
    DWORD tid = 0;

    non_english = (PRIMARYLANGID(GetUserDefaultLangID()) != LANG_ENGLISH);
    trace("non_english %d\n", non_english);

    CreateThread(NULL, 0, WorkerThread, &expectedOK, 0, &tid);
    MessageBoxA(NULL, "Message", msgbox_title, MB_OK);

    CreateThread(NULL, 0, WorkerThread, &expectedOkCancel, 0, &tid);
    MessageBoxA(NULL, "Message", msgbox_title, MB_OKCANCEL);

    CreateThread(NULL, 0, WorkerThread, &expectedAbortRetryIgnore, 0, &tid);
    MessageBoxA(NULL, "Message", msgbox_title, MB_ABORTRETRYIGNORE);

    CreateThread(NULL, 0, WorkerThread, &expectedYesNo, 0, &tid);
    MessageBoxA(NULL, "Message", msgbox_title, MB_YESNO);

    CreateThread(NULL, 0, WorkerThread, &expectedYesNoCancel, 0, &tid);
    MessageBoxA(NULL, "Message", msgbox_title, MB_YESNOCANCEL);

    CreateThread(NULL, 0, WorkerThread, &expectedRetryCancel, 0, &tid);
    MessageBoxA(NULL, "Message", msgbox_title, MB_RETRYCANCEL);

    CreateThread(NULL, 0, WorkerThread, &expectedCancelTryContinue, 0, &tid);
    MessageBoxA(NULL, "Message", msgbox_title, MB_CANCELTRYCONTINUE);
}

static void test_SaveRestoreFocus(void)
{
    HWND hDlg;
    HRSRC hResource;
    HANDLE hTemplate;
    DLGTEMPLATE* pTemplate;
    LONG_PTR foundId;
    HWND foundHwnd;

    /* create the dialog */
    hResource = FindResourceA(g_hinst, "MULTI_EDIT_DIALOG", (LPCSTR)RT_DIALOG);
    hTemplate = LoadResource(g_hinst, hResource);
    pTemplate = LockResource(hTemplate);

    hDlg = CreateDialogIndirectParamA(g_hinst, pTemplate, NULL, messageBoxFontDlgWinProc, 0);
    ok (hDlg != 0, "Failed to create test dialog.\n");

    foundId = GetWindowLongPtrA(GetFocus(), GWLP_ID);
    ok (foundId == 1000, "First edit box should have gained focus on dialog creation. Expected: %d, Found: %ld\n", 1000, foundId);

    SetFocus(GetNextDlgTabItem(hDlg, GetFocus(), FALSE));
    SendMessageA(hDlg, WM_ACTIVATE, MAKEWPARAM(WA_ACTIVE, 0), 0);
    foundId = GetWindowLongPtrA(GetFocus(), GWLP_ID);
    ok (foundId == 1001, "First edit box should have regained focus after dialog reactivation. Expected: %d, Found: %ld\n", 1001, foundId);
    SetFocus(GetNextDlgTabItem(hDlg, NULL, FALSE));

    /* de- then reactivate the dialog */
    SendMessageA(hDlg, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), 0);
    SendMessageA(hDlg, WM_ACTIVATE, MAKEWPARAM(WA_ACTIVE, 0), 0);

    foundId = GetWindowLongPtrA(GetFocus(), GWLP_ID);
    ok (foundId == 1000, "First edit box should have regained focus after dialog reactivation. Expected: %d, Found: %ld\n", 1000, foundId);

    /* select the next tabbable item */
    SetFocus(GetNextDlgTabItem(hDlg, GetFocus(), FALSE));

    foundId = GetWindowLongPtrA(GetFocus(), GWLP_ID);
    ok (foundId == 1001, "Second edit box should have gained focus. Expected: %d, Found: %ld\n", 1001, foundId);

    /* de- then reactivate the dialog */
    SendMessageA(hDlg, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), 0);
    SendMessageA(hDlg, WM_ACTIVATE, MAKEWPARAM(WA_ACTIVE, 0), 0);

    foundId = GetWindowLongPtrA(GetFocus(), GWLP_ID);
    ok (foundId == 1001, "Second edit box should have gained focus after dialog reactivation. Expected: %d, Found: %ld\n", 1001, foundId);

    /* set focus to the dialog */
    SetFocus(hDlg);

    foundId = GetWindowLongPtrA(GetFocus(), GWLP_ID);
    ok (foundId == 1000, "First edit box should have gained focus on dialog focus. Expected: %d, Found: %ld\n", 1000, foundId);

    /* select second tabbable item */
    SetFocus(GetNextDlgTabItem(hDlg, GetNextDlgTabItem(hDlg, NULL, FALSE), FALSE));

    foundId = GetWindowLongPtrA(GetFocus(), GWLP_ID);
    ok (foundId == 1001, "Second edit box should have gained focus. Expected: %d, Found: %ld\n", 1001, foundId);

    /* send WM_ACTIVATE message to already active dialog */
    SendMessageA(hDlg, WM_ACTIVATE, MAKEWPARAM(WA_ACTIVE, 0), 0);

    foundId = GetWindowLongPtrA(GetFocus(), GWLP_ID);
    ok (foundId == 1001, "Second edit box should have gained focus. Expected: %d, Found: %ld\n", 1001, foundId);

    /* disable the 2nd box */
    EnableWindow(GetFocus(), FALSE);

    foundHwnd = GetFocus();
    ok (foundHwnd == NULL, "Second edit box should have lost focus after being disabled. Expected: %p, Found: %p\n", NULL, foundHwnd);

    /* de- then reactivate the dialog */
    SendMessageA(hDlg, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), 0);
    SendMessageA(hDlg, WM_ACTIVATE, MAKEWPARAM(WA_ACTIVE, 0), 0);

    foundHwnd = GetFocus();
    ok (foundHwnd == NULL, "No controls should have gained focus after dialog reactivation. Expected: %p, Found: %p\n", NULL, foundHwnd);

    /* clean up */
    DestroyWindow(hDlg);
}

static INT_PTR CALLBACK timer_message_dlg_proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static int count;
    BOOL visible;

    switch (msg)
    {
        case WM_INITDIALOG:
            visible = GetWindowLongA(wnd, GWL_STYLE) & WS_VISIBLE;
            ok(!visible, "Dialog should not be visible.\n");
            SetTimer(wnd, 1, 100, NULL);
            Sleep(200);
            return FALSE;

        case WM_COMMAND:
            if (LOWORD(wparam) != IDCANCEL) return FALSE;
            EndDialog(wnd, LOWORD(wparam));
            return TRUE;

        case WM_TIMER:
            if (wparam != 1) return FALSE;
            visible = GetWindowLongA(wnd, GWL_STYLE) & WS_VISIBLE;
            if (!count++)
            {
                ok(!visible, "Dialog should not be visible.\n");
                PostMessageA(wnd, WM_USER, 0, 0);
            }
            else
            {
                ok(visible, "Dialog should be visible.\n");
                PostMessageA(wnd, WM_COMMAND, IDCANCEL, 0);
            }
            return TRUE;

        case WM_USER:
            visible = GetWindowLongA(wnd, GWL_STYLE) & WS_VISIBLE;
            ok(visible, "Dialog should be visible.\n");
            return TRUE;

        default:
            return FALSE;
    }
}

static void test_timer_message(void)
{
    DialogBoxA(g_hinst, "RADIO_TEST_DIALOG", NULL, timer_message_dlg_proc);
}

static LRESULT CALLBACK msgbox_hook_proc(INT code, WPARAM wParam, LPARAM lParam)
{
    if (code == HCBT_ACTIVATE)
    {
        HWND msgbox = (HWND)wParam, msghwnd;
        char text[64];

        if (msgbox)
        {
            text[0] = 0;
            GetWindowTextA(msgbox, text, sizeof(text));
            ok(!strcmp(text, "MSGBOX caption"), "Unexpected window text \"%s\"\n", text);

            msghwnd = GetDlgItem(msgbox, 0xffff);
            ok(msghwnd != NULL, "Expected static control\n");

            text[0] = 0;
            GetWindowTextA(msghwnd, text, sizeof(text));
            ok(!strcmp(text, "Text"), "Unexpected window text \"%s\"\n", text);

            SendDlgItemMessageA(msgbox, IDCANCEL, WM_LBUTTONDOWN, 0, 0);
            SendDlgItemMessageA(msgbox, IDCANCEL, WM_LBUTTONUP, 0, 0);
        }
    }

    return CallNextHookEx(NULL, code, wParam, lParam);
}

struct create_window_params
{
    BOOL owner;
    char caption[64];
    DWORD style;
};

static DWORD WINAPI create_window_thread(void *param)
{
    struct create_window_params *p = param;
    HWND owner = 0;

    if (p->owner)
    {
        owner = CreateWindowExA(0, "Static", NULL, WS_POPUP, 10, 10, 10, 10, 0, 0, 0, NULL);
        ok(owner != 0, "failed to create owner window\n");
    }

    MessageBoxA(owner, NULL, p->caption, p->style);

    if (owner) DestroyWindow(owner);

    return 0;
}

static HWND wait_for_window(const char *caption)
{
    HWND hwnd;
    DWORD timeout = 0;

    for (;;)
    {
        hwnd = FindWindowA(NULL, caption);
        if (hwnd) break;

        Sleep(50);
        timeout += 50;
        if (timeout > 3000)
        {
            ok(0, "failed to wait for a window %s\n", caption);
            break;
        }
    }

    Sleep(50);
    return hwnd;
}

static void test_MessageBox(void)
{
    static const struct
    {
        DWORD mb_style;
        DWORD ex_style;
    } test[] =
    {
        { MB_OK, 0 },
        { MB_OK | MB_TASKMODAL, 0 },
        { MB_OK | MB_SYSTEMMODAL, WS_EX_TOPMOST },
    };
    struct create_window_params params;
    HANDLE thread;
    DWORD tid, i;
    HHOOK hook;
    int ret;

    hook = SetWindowsHookExA(WH_CBT, msgbox_hook_proc, NULL, GetCurrentThreadId());

    ret = MessageBoxA(NULL, "Text", "MSGBOX caption", MB_OKCANCEL);
    ok(ret == IDCANCEL, "got %d\n", ret);

    UnhookWindowsHookEx(hook);

    sprintf(params.caption, "pid %08x, tid %08x, time %08x",
            GetCurrentProcessId(), GetCurrentThreadId(), GetCurrentTime());

    params.owner = FALSE;

    for (i = 0; i < sizeof(test)/sizeof(test[0]); i++)
    {
        HWND hwnd;
        DWORD ex_style;

        params.style = test[i].mb_style;

        thread = CreateThread(NULL, 0, create_window_thread, &params, 0, &tid);

        hwnd = wait_for_window(params.caption);
        ex_style = GetWindowLongA(hwnd, GWL_EXSTYLE);
        ok((ex_style & WS_EX_TOPMOST) == test[i].ex_style, "%d: got window ex_style %#x\n", i, ex_style);

        PostMessageA(hwnd, WM_COMMAND, IDCANCEL, 0);

        ok(WaitForSingleObject(thread, 5000) != WAIT_TIMEOUT, "thread failed to terminate\n");
        CloseHandle(thread);
    }

    params.owner = TRUE;

    for (i = 0; i < sizeof(test)/sizeof(test[0]); i++)
    {
        HWND hwnd;
        DWORD ex_style;

        params.style = test[i].mb_style;

        thread = CreateThread(NULL, 0, create_window_thread, &params, 0, &tid);

        hwnd = wait_for_window(params.caption);
        ex_style = GetWindowLongA(hwnd, GWL_EXSTYLE);
        ok((ex_style & WS_EX_TOPMOST) == test[i].ex_style, "%d: got window ex_style %#x\n", i, ex_style);

        PostMessageA(hwnd, WM_COMMAND, IDCANCEL, 0);

        ok(WaitForSingleObject(thread, 5000) != WAIT_TIMEOUT, "thread failed to terminate\n");
        CloseHandle(thread);
    }
}

static INT_PTR CALLBACK custom_test_dialog_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_INITDIALOG)
        EndDialog(hdlg, 0);

    return FALSE;
}

static void test_dialog_custom_data(void)
{
    DialogBoxA(g_hinst, "CUSTOM_TEST_DIALOG", NULL, custom_test_dialog_proc);
}

START_TEST(dialog)
{
    g_hinst = GetModuleHandleA (0);

    if (!RegisterWindowClasses()) assert(0);

    test_dialog_custom_data();
    test_GetNextDlgItem();
    test_IsDialogMessage();
    test_WM_NEXTDLGCTL();
    test_focus();
    test_GetDlgItem();
    test_GetDlgItemText();
    test_DialogBoxParam();
    test_DisabledDialogTest();
    test_MessageBoxFontTest();
    test_SaveRestoreFocus();
    test_timer_message();
    test_MessageBox();
    test_MessageBox_WM_COPY_Test();
}
