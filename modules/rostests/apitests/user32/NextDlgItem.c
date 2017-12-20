/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetNextDlgTabItem, GetNextDlgGroupItem
 * PROGRAMMERS:     Katayama Hirofumi MZ
 */

#include "precomp.h"

#include <windowsx.h>
#include <dlgs.h>

#define IDC_TEST1 1
#define IDC_TEST2 2
#define IDC_TEST3 3
#define IDC_END   9

static BOOL
OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_TEST1, 0), 0);
    return TRUE;
}

static const char *
GetNameFromID(INT ID)
{
    switch (ID)
    {
        case rad1:      return "rad1";
        case rad2:      return "rad2";
        case rad3:      return "rad3";
        case psh1:      return "psh1";
        case rad5:      return "rad5";
        case rad6:      return "rad6";
        case rad7:      return "rad7";
        case IDOK:      return "OK button";
        case IDCANCEL:  return "Cancel button";
        default:        return "(unknown)";
    }
}

static const char *
GetNameFromHWND(HWND hwnd)
{
    return GetNameFromID(GetDlgCtrlID(hwnd));
}

static void
DoTest1(HWND hwnd)
{
    HWND hCtrl;
    HWND hRad1, hRad2, hRad3, hPsh1, hRad5, hRad6, hRad7, hOK, hCancel;

    // check control IDs
    hCtrl = GetDlgItem(hwnd, rad1);
    ok(GetDlgCtrlID(hCtrl) == rad1, "\n");
    hCtrl = GetDlgItem(hwnd, rad2);
    ok(GetDlgCtrlID(hCtrl) == rad2, "\n");
    hCtrl = GetDlgItem(hwnd, rad3);
    ok(GetDlgCtrlID(hCtrl) == rad3, "\n");
    hCtrl = GetDlgItem(hwnd, psh1);
    ok(GetDlgCtrlID(hCtrl) == psh1, "\n");
    hCtrl = GetDlgItem(hwnd, rad5);
    ok(GetDlgCtrlID(hCtrl) == rad5, "\n");
    hCtrl = GetDlgItem(hwnd, rad6);
    ok(GetDlgCtrlID(hCtrl) == rad6, "\n");
    hCtrl = GetDlgItem(hwnd, rad7);
    ok(GetDlgCtrlID(hCtrl) == rad7, "\n");
    hCtrl = GetDlgItem(hwnd, IDOK);
    ok(GetDlgCtrlID(hCtrl) == IDOK, "\n");
    hCtrl = GetDlgItem(hwnd, IDCANCEL);
    ok(GetDlgCtrlID(hCtrl) == IDCANCEL, "\n");

    // get dialog items
    hRad1 = GetDlgItem(hwnd, rad1);
    hRad2 = GetDlgItem(hwnd, rad2);
    hRad3 = GetDlgItem(hwnd, rad3);
    hPsh1 = GetDlgItem(hwnd, psh1);
    hRad5 = GetDlgItem(hwnd, rad5);
    hRad6 = GetDlgItem(hwnd, rad6);
    hRad7 = GetDlgItem(hwnd, rad7);
    hOK = GetDlgItem(hwnd, IDOK);
    hCancel = GetDlgItem(hwnd, IDCANCEL);

    // next
    hCtrl = GetNextDlgTabItem(hwnd, hRad1, FALSE);
    ok(hCtrl == hRad2, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad2, FALSE);
    ok(hCtrl == hRad3, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad3, FALSE);
    ok(hCtrl == hPsh1, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hPsh1, FALSE);
    ok(hCtrl == hRad5, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad5, FALSE);
    ok(hCtrl == hRad6, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad6, FALSE);
    ok(hCtrl == hRad7, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad7, FALSE);
    ok(hCtrl == hOK, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hOK, FALSE);
    ok(hCtrl == hCancel, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hCancel, FALSE);
    ok(hCtrl == hRad1, "hCtrl was %s\n", GetNameFromHWND(hCtrl));

    // prev
    hCtrl = GetNextDlgTabItem(hwnd, hRad1, TRUE);
    ok(hCtrl == hCancel, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad2, TRUE);
    ok(hCtrl == hRad1, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad3, TRUE);
    ok(hCtrl == hRad2, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hPsh1, TRUE);
    ok(hCtrl == hRad3, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad5, TRUE);
    ok(hCtrl == hPsh1, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad6, TRUE);
    ok(hCtrl == hRad5, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad7, TRUE);
    ok(hCtrl == hRad6, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hOK, TRUE);
    ok(hCtrl == hRad7, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hCancel, TRUE);
    ok(hCtrl == hOK, "hCtrl was %s\n", GetNameFromHWND(hCtrl));

    // hide
    ok(ShowWindow(hRad3, SW_HIDE) != 0, "\n");
    ok(ShowWindow(hRad6, SW_HIDE) != 0, "\n");

    // next with hidden
    hCtrl = GetNextDlgTabItem(hwnd, hRad1, FALSE);
    ok(hCtrl == hRad2, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad2, FALSE);
    ok(hCtrl == hPsh1, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hPsh1, FALSE);
    ok(hCtrl == hRad5, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad5, FALSE);
    ok(hCtrl == hRad7, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad7, FALSE);
    ok(hCtrl == hOK, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hOK, FALSE);
    ok(hCtrl == hCancel, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hCancel, FALSE);
    ok(hCtrl == hRad1, "hCtrl was %s\n", GetNameFromHWND(hCtrl));

    // prev with hidden
    hCtrl = GetNextDlgTabItem(hwnd, hRad1, TRUE);
    ok(hCtrl == hCancel, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad2, TRUE);
    ok(hCtrl == hRad1, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hPsh1, TRUE);
    ok(hCtrl == hRad2, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad5, TRUE);
    ok(hCtrl == hPsh1, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad7, TRUE);
    ok(hCtrl == hRad5, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hOK, TRUE);
    ok(hCtrl == hRad7, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hCancel, TRUE);
    ok(hCtrl == hOK, "hCtrl was %s\n", GetNameFromHWND(hCtrl));

    // show
    ShowWindow(hRad3, SW_SHOWNOACTIVATE);
    ShowWindow(hRad6, SW_SHOWNOACTIVATE);

    // next again
    hCtrl = GetNextDlgTabItem(hwnd, hRad1, FALSE);
    ok(hCtrl == hRad2, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad2, FALSE);
    ok(hCtrl == hRad3, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad3, FALSE);
    ok(hCtrl == hPsh1, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hPsh1, FALSE);
    ok(hCtrl == hRad5, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad5, FALSE);
    ok(hCtrl == hRad6, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad6, FALSE);
    ok(hCtrl == hRad7, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad7, FALSE);
    ok(hCtrl == hOK, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hOK, FALSE);
    ok(hCtrl == hCancel, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hCancel, FALSE);
    ok(hCtrl == hRad1, "hCtrl was %s\n", GetNameFromHWND(hCtrl));

    // prev again
    hCtrl = GetNextDlgTabItem(hwnd, hRad1, TRUE);
    ok(hCtrl == hCancel, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad2, TRUE);
    ok(hCtrl == hRad1, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad3, TRUE);
    ok(hCtrl == hRad2, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hPsh1, TRUE);
    ok(hCtrl == hRad3, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad5, TRUE);
    ok(hCtrl == hPsh1, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad6, TRUE);
    ok(hCtrl == hRad5, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hRad7, TRUE);
    ok(hCtrl == hRad6, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hOK, TRUE);
    ok(hCtrl == hRad7, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgTabItem(hwnd, hCancel, TRUE);
    ok(hCtrl == hOK, "hCtrl was %s\n", GetNameFromHWND(hCtrl));
}

static void
DoTest2(HWND hwnd)
{
    HWND hCtrl;
    HWND hRad1, hRad2, hRad3, hPsh1, hRad5, hRad6, hRad7, hOK, hCancel;

    // check control IDs
    hCtrl = GetDlgItem(hwnd, rad1);
    ok(GetDlgCtrlID(hCtrl) == rad1, "\n");
    hCtrl = GetDlgItem(hwnd, rad2);
    ok(GetDlgCtrlID(hCtrl) == rad2, "\n");
    hCtrl = GetDlgItem(hwnd, rad3);
    ok(GetDlgCtrlID(hCtrl) == rad3, "\n");
    hCtrl = GetDlgItem(hwnd, psh1);
    ok(GetDlgCtrlID(hCtrl) == psh1, "\n");
    hCtrl = GetDlgItem(hwnd, rad5);
    ok(GetDlgCtrlID(hCtrl) == rad5, "\n");
    hCtrl = GetDlgItem(hwnd, rad6);
    ok(GetDlgCtrlID(hCtrl) == rad6, "\n");
    hCtrl = GetDlgItem(hwnd, rad7);
    ok(GetDlgCtrlID(hCtrl) == rad7, "\n");
    hCtrl = GetDlgItem(hwnd, IDOK);
    ok(GetDlgCtrlID(hCtrl) == IDOK, "\n");
    hCtrl = GetDlgItem(hwnd, IDCANCEL);
    ok(GetDlgCtrlID(hCtrl) == IDCANCEL, "\n");

    // get dialog items
    hRad1 = GetDlgItem(hwnd, rad1);
    hRad2 = GetDlgItem(hwnd, rad2);
    hRad3 = GetDlgItem(hwnd, rad3);
    hPsh1 = GetDlgItem(hwnd, psh1);
    hRad5 = GetDlgItem(hwnd, rad5);
    hRad6 = GetDlgItem(hwnd, rad6);
    hRad7 = GetDlgItem(hwnd, rad7);
    hOK = GetDlgItem(hwnd, IDOK);
    hCancel = GetDlgItem(hwnd, IDCANCEL);

    // group next
    hCtrl = GetNextDlgGroupItem(hwnd, hRad1, FALSE);
    ok(hCtrl == hRad2, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad2, FALSE);
    ok(hCtrl == hRad3, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad3, FALSE);
    ok(hCtrl == hPsh1, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hPsh1, FALSE);
    ok(hCtrl == hRad5, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad5, FALSE);
    ok(hCtrl == hRad6, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad6, FALSE);
    ok(hCtrl == hRad7, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad7, FALSE);
    ok(hCtrl == hOK, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hOK, FALSE);
    ok(hCtrl == hCancel, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hCancel, FALSE);
    ok(hCtrl == hRad1, "hCtrl is %s\n", GetNameFromHWND(hCtrl));

    // group prev
    hCtrl = GetNextDlgGroupItem(hwnd, hRad1, TRUE);
    ok(hCtrl == hCancel, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad2, TRUE);
    ok(hCtrl == hRad1, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad3, TRUE);
    ok(hCtrl == hRad2, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hPsh1, TRUE);
    ok(hCtrl == hRad3, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad5, TRUE);
    ok(hCtrl == hPsh1, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad6, TRUE);
    ok(hCtrl == hRad5, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad7, TRUE);
    ok(hCtrl == hRad6, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hOK, TRUE);
    ok(hCtrl == hRad7, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hCancel, TRUE);
    ok(hCtrl == hOK, "hCtrl is %s\n", GetNameFromHWND(hCtrl));

    // hide
    ok(ShowWindow(hRad3, SW_HIDE) != 0, "\n");
    ok(ShowWindow(hRad6, SW_HIDE) != 0, "\n");

    // group next with hidden
    hCtrl = GetNextDlgGroupItem(hwnd, hRad1, FALSE);
    ok(hCtrl == hRad2, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad2, FALSE);
    ok(hCtrl == hPsh1, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hPsh1, FALSE);
    ok(hCtrl == hRad5, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad5, FALSE);
    ok(hCtrl == hRad7, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad7, FALSE);
    ok(hCtrl == hOK, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hOK, FALSE);
    ok(hCtrl == hCancel, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hCancel, FALSE);
    ok(hCtrl == hRad1, "hCtrl is %s\n", GetNameFromHWND(hCtrl));

    // group prev with hidden
    hCtrl = GetNextDlgGroupItem(hwnd, hRad1, TRUE);
    ok(hCtrl == hCancel, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad2, TRUE);
    ok(hCtrl == hRad1, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hPsh1, TRUE);
    ok(hCtrl == hRad2, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad5, TRUE);
    ok(hCtrl == hPsh1, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hRad7, TRUE);
    ok(hCtrl == hRad5, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hOK, TRUE);
    ok(hCtrl == hRad7, "hCtrl is %s\n", GetNameFromHWND(hCtrl));
    hCtrl = GetNextDlgGroupItem(hwnd, hCancel, TRUE);
    ok(hCtrl == hOK, "hCtrl is %s\n", GetNameFromHWND(hCtrl));

    // show
    ShowWindow(hRad3, SW_SHOWNOACTIVATE);
    ShowWindow(hRad6, SW_SHOWNOACTIVATE);
}

static POINT
GetCtrlCenter(HWND hCtrl)
{
    RECT rc;
    POINT pt;

    GetClientRect(hCtrl, &rc);
    pt.x = (rc.left + rc.right) / 2;
    pt.y = (rc.top + rc.bottom) / 2;
    return pt;
}

static void
DoTest3(HWND hwnd)
{
    HWND hCtrl;
    POINT pt;

    CheckDlgButton(hwnd, rad1, BST_CHECKED);
    CheckDlgButton(hwnd, rad5, BST_CHECKED);

    ok(IsDlgButtonChecked(hwnd, rad1) == BST_CHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad2) == BST_UNCHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad3) == BST_UNCHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad5) == BST_CHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad6) == BST_UNCHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad7) == BST_UNCHECKED, "\n");

    hCtrl = GetDlgItem(hwnd, rad1);
    pt = GetCtrlCenter(hCtrl);
    SendMessage(hCtrl, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));
    SendMessage(hCtrl, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));

    ok(IsDlgButtonChecked(hwnd, rad1) == BST_CHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad2) == BST_UNCHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad3) == BST_UNCHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad5) == BST_UNCHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad6) == BST_UNCHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad7) == BST_UNCHECKED, "\n");

    hCtrl = GetDlgItem(hwnd, rad5);
    pt = GetCtrlCenter(hCtrl);
    SendMessage(hCtrl, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));
    SendMessage(hCtrl, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));

    ok(IsDlgButtonChecked(hwnd, rad1) == BST_UNCHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad2) == BST_UNCHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad3) == BST_UNCHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad5) == BST_CHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad6) == BST_UNCHECKED, "\n");
    ok(IsDlgButtonChecked(hwnd, rad7) == BST_UNCHECKED, "\n");
}

static void
OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDC_TEST1:
            DoTest1(hwnd);
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_TEST2, 0), 0);
            break;
        case IDC_TEST2:
            DoTest2(hwnd);
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_TEST3, 0), 0);
            break;
        case IDC_TEST3:
            DoTest3(hwnd);
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_END, 0), 0);
            break;
        case IDC_END:
            EndDialog(hwnd, IDOK);
            break;
    }
}

INT_PTR CALLBACK
RadioButtonDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
    }
    return 0;
}

START_TEST(NextDlgItem)
{
    HMODULE hMod = GetModuleHandle(NULL);
    ok(hMod != NULL, "\n");
    DialogBox(hMod, TEXT("NEXTDLGITEM"), NULL, RadioButtonDialogProc);
}
