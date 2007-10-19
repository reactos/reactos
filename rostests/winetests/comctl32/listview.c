/*
 * ListView tests
 *
 * Copyright 2006 Mike McCormack for CodeWeavers
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

#include <stdio.h>
#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"

static void test_images(void)
{
    HWND hwnd, hwndparent = 0;
    DWORD r;
    LVITEM item;
    HIMAGELIST himl;
    HBITMAP hbmp;
    RECT r1, r2;
    static CHAR hello[] = "hello";

    himl = ImageList_Create(40, 40, 0, 4, 4);
    ok(himl != NULL, "failed to create imagelist\n");

    hbmp = CreateBitmap(40, 40, 1, 1, NULL);
    ok(hbmp != NULL, "failed to create bitmap\n");

    r = ImageList_Add(himl, hbmp, 0);
    ok(r == 0, "should be zero\n");

    hwnd = CreateWindowEx(0, "SysListView32", "foo", LVS_OWNERDRAWFIXED,
                10, 10, 100, 200, hwndparent, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create listview window\n");

    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, 0x940);
    ok(r == 0, "should return zero\n");

    r = SendMessage(hwnd, LVM_SETIMAGELIST, 0, (LPARAM)himl);
    ok(r == 0, "should return zero\n");

    r = SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELONG(100,50));
    /* returns dimensions */

    r = SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);
    ok(r == 0, "should be zero items\n");

    item.mask = LVIF_IMAGE | LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 1;
    item.iImage = 0;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    ok(r == -1, "should fail\n");

    item.iSubItem = 0;
    item.pszText = hello;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    ok(r == 0, "should not fail\n");

    memset(&r1, 0, sizeof r1);
    r1.left = LVIR_ICON;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM) &r1);

    r = SendMessage(hwnd, LVM_DELETEALLITEMS, 0, 0);
    ok(r == TRUE, "should not fail\n");

    item.iSubItem = 0;
    item.pszText = hello;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    ok(r == 0, "should not fail\n");

    memset(&r2, 0, sizeof r2);
    r2.left = LVIR_ICON;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM) &r2);

    ok(!memcmp(&r1, &r2, sizeof r1), "rectangle should be the same\n");

    DestroyWindow(hwnd);
}

static void test_checkboxes(void)
{
    HWND hwnd, hwndparent = 0;
    LVITEMA item;
    DWORD r;
    static CHAR text[]  = "Text",
                text2[] = "Text2",
                text3[] = "Text3";

    hwnd = CreateWindowEx(0, "SysListView32", "foo", LVS_REPORT,
                10, 10, 100, 200, hwndparent, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create listview window\n");

    /* first without LVS_EX_CHECKBOXES set and an item and check that state is preserved */
    item.mask = LVIF_TEXT | LVIF_STATE;
    item.stateMask = 0xffff;
    item.state = 0xfccc;
    item.iItem = 0;
    item.iSubItem = 0;
    item.pszText = text;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 0, "ret %ld\n", r);

    item.iItem = 0;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0xfccc, "state %x\n", item.state);

    /* Don't set LVIF_STATE */
    item.mask = LVIF_TEXT;
    item.stateMask = 0xffff;
    item.state = 0xfccc;
    item.iItem = 1;
    item.iSubItem = 0;
    item.pszText = text;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 1, "ret %ld\n", r);

    item.iItem = 1;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0, "state %x\n", item.state);

    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    ok(r == 0, "should return zero\n");

    /* Having turned on checkboxes, check that all existing items are set to 0x1000 (unchecked) */
    item.iItem = 0;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x1ccc, "state %x\n", item.state);

    /* Now add an item without specifying a state and check that it's state goes to 0x1000 */
    item.iItem = 2;
    item.mask = LVIF_TEXT;
    item.state = 0;
    item.pszText = text2;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 2, "ret %ld\n", r);

    item.iItem = 2;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x1000, "state %x\n", item.state);

    /* Add a further item this time specifying a state and still it's state goes to 0x1000 */
    item.iItem = 3;
    item.mask = LVIF_TEXT | LVIF_STATE;
    item.stateMask = 0xffff;
    item.state = 0x2aaa;
    item.pszText = text3;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 3, "ret %ld\n", r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x1aaa, "state %x\n", item.state);

    /* Set an item's state to checked */
    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xf000;
    item.state = 0x2000;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x2aaa, "state %x\n", item.state);

    /* Set the style again and check that doesn't change an item's state */
    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    ok(r == LVS_EX_CHECKBOXES, "ret %lx\n", r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x2aaa, "state %x\n", item.state);

    /* Unsetting the checkbox extended style doesn't change an item's state */
    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, 0);
    ok(r == LVS_EX_CHECKBOXES, "ret %lx\n", r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x2aaa, "state %x\n", item.state);

    /* Now setting the style again will change an item's state */
    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    ok(r == 0, "ret %lx\n", r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x1aaa, "state %x\n", item.state);

    DestroyWindow(hwnd);
}

START_TEST(listview)
{
    INITCOMMONCONTROLSEX icc;

    icc.dwICC = 0;
    icc.dwSize = sizeof icc;
    InitCommonControlsEx(&icc);

    test_images();
    test_checkboxes();
}
