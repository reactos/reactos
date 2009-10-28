/*
 * Test winmm mci
 *
 * Copyright 2006 Jan Zerebecki
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

#include "wine/test.h"
#include "winuser.h"
#include "mmsystem.h"

START_TEST(mci)
{
    int err;
    const char command_open[] = "open new type waveaudio alias mysound";
    const char command_close_my[] = "close mysound notify";
    const char command_close_all[] = "close all notify";
    const char command_sysinfo[] = "sysinfo waveaudio quantity open";
    MSG msg;
    char buf[1024];
    HWND hwnd;

    hwnd = CreateWindowExA(0, "static", "winmm test", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);

    err = mciSendString(command_open, NULL, 0, NULL);
    ok(!err,"mciSendString(%s, NULL, 0 , NULL) returned error: %d\n", command_open, err);

    err = mciSendString(command_close_my, NULL, 0, hwnd);
    ok(!err,"mciSendString(%s, NULL, 0 , NULL) returned error: %d\n", command_close_my, err);

    ok(PeekMessageA( &msg, hwnd, 0, 0, PM_REMOVE ), "PeekMessage should succeed\n");
    ok(msg.hwnd == hwnd, "Didn't get the handle to our test window\n");
    ok(msg.message == MM_MCINOTIFY, "got %04x instead of MM_MCINOTIFY\n", msg.message);
    ok(msg.wParam == MCI_NOTIFY_SUCCESSFUL, "got %08lx instead of MCI_NOTIFY_SUCCESSFUL\n", msg.wParam);

    err = mciSendString(command_close_all, NULL, 0, NULL);
    todo_wine ok(!err,"mciSendString(%s, NULL, 0 , NULL) returned error: %d\n", command_close_all, err);

    memset(buf, 0, sizeof(buf));
    err = mciSendString(command_close_all, buf, sizeof(buf), hwnd);
    todo_wine ok(!err,"mciSendString(%s, buf, sizeof(buf) , NULL) returned error: %d\n", command_close_all, err);
    ok(buf[0] == 0, "mciSendString(%s, buf, sizeof(buf) , NULL) changed output buffer: %s\n", command_close_all, buf);

    memset(buf, 0, sizeof(buf));
    err = mciSendString(command_sysinfo, buf, sizeof(buf), NULL);
    ok(!err,"mciSendString(%s, buf, sizeof(buf) , NULL) returned error: %d\n", command_sysinfo, err);
    todo_wine ok(buf[0] == '0' && buf[1] == 0, "mciSendString(%s, buf, sizeof(buf) , NULL), expected output buffer '0', got: '%s'\n", command_sysinfo, buf);

    err = mciSendCommand(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_NOTIFY, 0);
    todo_wine ok(err == MCIERR_INVALID_DEVICE_ID ||
        broken(!err), /* Win9x and WinMe */
        "mciSendCommand(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_NOTIFY, NULL) returned %d instead of %d\n",
        err, MCIERR_INVALID_DEVICE_ID);

    DestroyWindow(hwnd);
}
