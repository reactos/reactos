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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include "winuser.h"

#define MAXHWNDS 1024
static HWND hwnd [MAXHWNDS];
static int numwnds=1; /* 0 is reserved for null */

/* Global handles */
static HINSTANCE g_hinst;                          /* This application's HINSTANCE */
static HWND g_hwndMain, g_hwndButton1, g_hwndButton2, g_hwndButtonCancel;
static int g_terminated;

typedef struct {
    int id;
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

static BOOL CreateWindows (HINSTANCE hinst)
{
    const h_entry *p = hierarchy;

    while (p->id != 0)
    {
        DWORD style, exstyle;
        char ctrlname[9];

        /* Basically assert that the hierarchy is valid and track the
         * maximum control number
         */
        if (p->id >= numwnds)
        {
            if (p->id >=  sizeof(hwnd)/sizeof(hwnd[0]))
            {
                trace ("Control %d is out of range\n", p->id);
                return FALSE;
            }
            else
                numwnds = p->id+1;
        }
        if (p->id <= 0)
        {
            trace ("Control %d is out of range\n", p->id);
            return FALSE;
        }
        if (hwnd[p->id] != 0)
        {
            trace ("Control %d is used more than once\n", p->id);
            return FALSE;
        }

        /* Create the control */
        sprintf (ctrlname, "ctrl%4.4d", p->id);
        hwnd[p->id] = CreateWindowEx (p->exstyle, TEXT(p->parent ? "static" : "GetNextDlgItemWindowClass"), TEXT(ctrlname), p->style, 10, 10, 10, 10, hwnd[p->parent], p->parent ? (HMENU) (2000 + p->id) : 0, hinst, 0);
        if (!hwnd[p->id])
        {
            trace ("Failed to create control %d\n", p->id);
            return FALSE;
        }

        /* Check that the styles are as we specified (except the main one
         * which is quite frequently messed up).  If this keeps breaking then 
         * we could mask out the bits that don't concern us.
         */
        if (p->parent)
        {
            style = GetWindowLong (hwnd[p->id], GWL_STYLE);
            exstyle = GetWindowLong (hwnd[p->id], GWL_EXSTYLE);
            if (style == p->style && exstyle == p->exstyle)
            {
                trace ("Style mismatch at %d: %8.8lx %8.8lx cf %8.8lx %8.8lx\n", p->id, style, exstyle, p->style, p->exstyle);
            }
        }
        p++;
    }

    return TRUE;
}

/* Form the lParam of a WM_KEYDOWN message */
static DWORD KeyDownData (int repeat, int scancode, int extended, int wasdown)
{
    return ((repeat & 0x0000FFFF) | ((scancode & 0x00FF) >> 16) |
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
    int i;
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
 *      Check it skips invisible, diabled and both.
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

static void GetNextDlgItemTest (void)
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
        if (p->isok)
        {
            ok (a == hwnd[p->res], "Test %d: %s %s item of %d in %d was %d instead of %d\n", p->test, p->prev ? "Prev" : "Next", p->tab ? "Tab" : "Group", p->ctl, p->dlg, id(a), p->res);
        }
        else
        {
            todo_wine
            {
                ok (a == hwnd[p->res], "Test %d: %s %s item of %d in %d was actually  %d matching expected %d\n", p->test, p->prev ? "Prev" : "Next", p->tab ? "Tab" : "Group", p->ctl, p->dlg, id(a), p->res);
            }
        }
        p++;
    }
}

/*
 *  OnMainWindowCreate
 */
static BOOL OnMainWindowCreate (HWND hwnd, LPCREATESTRUCT lpcs)
{
    g_hwndButton1 = CreateWindow (TEXT("button"), TEXT("Button &1"),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON | BS_TEXT,
            10, 10, 80, 80, hwnd, (HMENU)100, g_hinst, 0);
    if (!g_hwndButton1) return FALSE;

    g_hwndButton2 = CreateWindow (TEXT("button"), TEXT("Button &2"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_TEXT,
            110, 10, 80, 80, hwnd, (HMENU)200, g_hinst, 0);
    if (!g_hwndButton2) return FALSE;

    g_hwndButtonCancel = CreateWindow (TEXT("button"), TEXT("Cancel"),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_TEXT,
            210, 10, 80, 80, hwnd, (HMENU)IDCANCEL, g_hinst, 0);
    if (!g_hwndButtonCancel) return FALSE;

    return TRUE;
}


static LRESULT CALLBACK main_window_procA (HWND hwnd, UINT uiMsg, WPARAM wParam,
        LPARAM lParam)
{
    LRESULT result;
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
            break;
    }

    result=DefWindowProcA (hwnd, uiMsg, wParam, lParam);
    return result;
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
    cls.hCursor = LoadCursorA (NULL, IDC_ARROW);
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "GetNextDlgItemWindowClass";

    if (!RegisterClassA (&cls)) return FALSE;

    cls.lpfnWndProc = main_window_procA;
    cls.lpszClassName = "IsDialogMessageWindowClass";

    if (!RegisterClassA (&cls)) return FALSE;

    return TRUE;
}

static void IsDialogMessageWTest (void)
{
    MSG msg;

    g_hwndMain = CreateWindow ("IsDialogMessageWindowClass", "IsDialogMessageWindowClass",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, g_hinst, 0);

    assert (g_hwndMain);
    assert (g_hwndButton1);
    assert (g_hwndButtonCancel);

    /* The focus should initially be nowhere.  The first TAB should take it
     * to the first button.  The second TAB should take it to the Cancel
     * button.
     */
    FormTabMsg (&msg, g_hwndMain);
    ok (IsDialogMessage (g_hwndMain, &msg), "Did not handle first TAB\n");
    ok ((GetFocus() == g_hwndButton1), "Focus did not move to first button\n");
    FormTabMsg (&msg, g_hwndButton1);
    ok (IsDialogMessage (g_hwndMain, &msg), "Did not handle second TAB\n");
    ok ((GetFocus() == g_hwndButtonCancel),
            "Focus did not move to cancel button\n");
    FormEnterMsg (&msg, g_hwndButtonCancel);
    ok (IsDialogMessage (g_hwndMain, &msg), "Did not handle the ENTER\n");
    ok (g_terminated, "ENTER did not terminate\n");
}

START_TEST(dialog)
{
    g_hinst = GetModuleHandleA (0);

    if (!RegisterWindowClasses()) assert(0);

    GetNextDlgItemTest();
    IsDialogMessageWTest();
}