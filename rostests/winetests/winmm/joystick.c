/*
 * Unit tests for joystick APIs
 *
 * Copyright 2014 Bruno Jesus
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "mmsystem.h"
#include "wine/test.h"

static HWND window;

static LRESULT CALLBACK proc_window(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void create_window(void)
{
    const char name[]  = "Joystick Test";
    WNDCLASSA wc;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = proc_window;
    wc.hInstance     = 0;
    wc.lpszClassName = name;
    RegisterClassA(&wc);
    window = CreateWindowExA(0, name, name, WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                             NULL, NULL, NULL, NULL);
    ok(window != NULL, "Expected CreateWindowEx to work, error %d\n", GetLastError());
}

static void destroy_window(void)
{
    DestroyWindow(window);
    window = NULL;
}

static void test_api(void)
{
    MMRESULT ret;
    JOYCAPSA jc;
    JOYCAPSW jcw;
    JOYINFO info;
    union _infoex
    {
        JOYINFOEX ex;
        char buffer[sizeof(JOYINFOEX) * 2];
    } infoex;
    UINT i, par, devices, joyid, win98 = 0, win8 = 0;
    UINT period[] = {0, 1, 9, 10, 100, 1000, 1001, 10000, 65535, 65536, 0xFFFFFFFF};
    UINT threshold_error = 0x600, period_win8_error = 0x7CE;
    UINT flags[] = { JOY_RETURNALL, JOY_RETURNBUTTONS, JOY_RETURNCENTERED, JOY_RETURNPOV,
                     JOY_RETURNPOVCTS, JOY_RETURNR, JOY_RETURNRAWDATA, JOY_RETURNU,
                     JOY_RETURNV, JOY_RETURNX, JOY_RETURNY, JOY_RETURNZ };

    devices = joyGetNumDevs();
    joyid = -1;
    /* joyGetNumDevs does NOT return the number of joysticks connected, only slots in the OS */
    for (i = 0; i < devices; i++)
    {
        memset(&jc, 0, sizeof(jc));
        ret = joyGetDevCapsA(JOYSTICKID1 + i, &jc, sizeof(jc));
        if (ret == JOYERR_NOERROR)
        {
            joyid = JOYSTICKID1 + i;
            trace("Joystick[%d] - name: '%s', axes: %d, buttons: %d, period range: %d - %d\n",
                  JOYSTICKID1 + i, jc.szPname, jc.wNumAxes, jc.wNumButtons, jc.wPeriodMin, jc.wPeriodMax);
            ret = joyGetDevCapsW(JOYSTICKID1 + i, &jcw, sizeof(jcw));
            if (ret != MMSYSERR_NOTSUPPORTED) /* Win 98 */
            {
                ok(ret == JOYERR_NOERROR, "Expected %d, got %d\n", JOYERR_NOERROR, ret);
                ok(jc.wNumAxes == jcw.wNumAxes, "Expected %d == %d\n", jc.wNumAxes, jcw.wNumAxes);
                ok(jc.wNumButtons == jcw.wNumButtons, "Expected %d == %d\n", jc.wNumButtons, jcw.wNumButtons);
            }
            else win98++;
            break;
        }
        else
        {
            ok(ret == JOYERR_PARMS, "Expected %d, got %d\n", JOYERR_PARMS, ret);
            ret = joyGetDevCapsW(JOYSTICKID1 + i, &jcw, sizeof(jcw));
            ok(ret == JOYERR_PARMS || (ret == MMSYSERR_NOTSUPPORTED) /* Win 98 */,
               "Expected %d, got %d\n", JOYERR_PARMS, ret);
        }
    }
    /* Test invalid joystick - If no joystick is present the driver is not initialized,
     * so a NODRIVER error is returned, if at least one joystick is present the error is
     * about invalid parameters. */
    ret = joyGetDevCapsA(joyid + devices, &jc, sizeof(jc));
    ok(ret == MMSYSERR_NODRIVER || ret == JOYERR_PARMS,
       "Expected %d or %d, got %d\n", MMSYSERR_NODRIVER, JOYERR_PARMS, ret);

    if (joyid == -1)
    {
        skip("This test requires a real joystick.\n");
        return;
    }

    /* Capture tests */
    ret = joySetCapture(NULL, joyid, 100, FALSE);
    ok(ret == JOYERR_PARMS || broken(win98 && ret == MMSYSERR_INVALPARAM) /* Win 98 */,
       "Expected %d, got %d\n", JOYERR_PARMS, ret);
    ret = joySetCapture(window, joyid, 100, FALSE);
    ok(ret == JOYERR_NOERROR, "Expected %d, got %d\n", JOYERR_NOERROR, ret);
    ret = joySetCapture(window, joyid, 100, FALSE); /* double capture */
    if (ret == JOYERR_NOCANDO)
    {
        todo_wine
        ok(broken(1), "Expected double capture using joySetCapture to work\n");
        if (!win98 && broken(1)) win8++; /* Windows 98 or 8 cannot cope with that */
    }
    else ok(ret == JOYERR_NOERROR, "Expected %d, got %d\n", JOYERR_NOERROR, ret);
    ret = joyReleaseCapture(joyid);
    ok(ret == JOYERR_NOERROR, "Expected %d, got %d\n", JOYERR_NOERROR, ret);
    ret = joyReleaseCapture(joyid);
    ok(ret == JOYERR_NOERROR, "Expected %d, got %d\n", JOYERR_NOERROR, ret); /* double release */

    /* Try some unusual period values for joySetCapture and unusual threshold values for joySetThreshold.
     * Windows XP allows almost all test values, Windows 8 will return error on most test values, Windows
     * 98 allows anything but cuts the values to their maximum supported values internally. */
    for (i = 0; i < sizeof(period) / sizeof(period[0]); i++)
    {
        ret = joySetCapture(window, joyid, period[i], FALSE);
        if (win8 && ((1 << i) & period_win8_error))
            ok(ret == JOYERR_NOCANDO, "Test [%d]: Expected %d, got %d\n", i, JOYERR_NOCANDO, ret);
        else
            ok(ret == JOYERR_NOERROR, "Test [%d]: Expected %d, got %d\n", i, JOYERR_NOERROR, ret);
        ret = joyReleaseCapture(joyid);
        ok(ret == JOYERR_NOERROR, "Test [%d]: Expected %d, got %d\n", i, JOYERR_NOERROR, ret);
        /* Reuse the periods to test the threshold */
        ret = joySetThreshold(joyid, period[i]);
        if (!win98 && (1 << i) & threshold_error)
            ok(ret == MMSYSERR_INVALPARAM, "Test [%d]: Expected %d, got %d\n", i, MMSYSERR_INVALPARAM, ret);
        else
            ok(ret == JOYERR_NOERROR, "Test [%d]: Expected %d, got %d\n", i, JOYERR_NOERROR, ret);
        par = 0xdead;
        ret = joyGetThreshold(joyid, &par);
        ok(ret == JOYERR_NOERROR, "Test [%d]: Expected %d, got %d\n", i, JOYERR_NOERROR, ret);
        if (!win98 || i < 8)
        {
            if ((1 << i) & threshold_error)
                ok(par == period[8], "Test [%d]: Expected %d, got %d\n", i, period[8], par);
            else
                ok(par == period[i], "Test [%d]: Expected %d, got %d\n", i, period[i], par);
        }
    }

    /* Position tests */
    ret = joyGetPos(joyid, NULL);
    ok(ret == MMSYSERR_INVALPARAM, "Expected %d, got %d\n", MMSYSERR_INVALPARAM, ret);
    ret = joyGetPos(joyid, &info);
    ok(ret == JOYERR_NOERROR, "Expected %d, got %d\n", JOYERR_NOERROR, ret);
    ret = joyGetPosEx(joyid, NULL);
    ok(ret == MMSYSERR_INVALPARAM || broken(win8 && ret == JOYERR_PARMS) /* Win 8 */,
       "Expected %d, got %d\n", MMSYSERR_INVALPARAM, ret);
    memset(&infoex, 0, sizeof(infoex));
    ret = joyGetPosEx(joyid, &infoex.ex);
    ok(ret == JOYERR_PARMS || broken(win98 && ret == MMSYSERR_INVALPARAM),
       "Expected %d, got %d\n", JOYERR_PARMS, ret);
    infoex.ex.dwSize = sizeof(infoex.ex);
    ret = joyGetPosEx(joyid, &infoex.ex);
    ok(ret == JOYERR_NOERROR, "Expected %d, got %d\n", JOYERR_NOERROR, ret);
    infoex.ex.dwSize = sizeof(infoex.ex) - 1;
    ret = joyGetPosEx(joyid, &infoex.ex);
    ok(ret == JOYERR_PARMS || broken(win98 && ret == MMSYSERR_INVALPARAM),
       "Expected %d, got %d\n", JOYERR_PARMS, ret);
    infoex.ex.dwSize = sizeof(infoex);
    ret = joyGetPosEx(joyid, &infoex.ex);
    ok(ret == JOYERR_NOERROR, "Expected %d, got %d\n", JOYERR_NOERROR, ret);

    infoex.ex.dwSize = sizeof(infoex.ex);
    for (i = 0; i < sizeof(flags) / sizeof(flags[0]); i++)
    {
        infoex.ex.dwFlags = flags[i];
        ret = joyGetPosEx(joyid, &infoex.ex);
        ok(ret == JOYERR_NOERROR, "Expected %d, got %d\n", JOYERR_NOERROR, ret);
    }

    /* the interactive tests spans for 15 seconds, a 500ms polling is used to get
     * changes in the joystick. */
    if (winetest_interactive)
    {
#define MAX_TIME 15000
        DWORD tick = GetTickCount(), spent;
        infoex.ex.dwSize = sizeof(infoex.ex);
        infoex.ex.dwFlags = JOY_RETURNALL;
        do
        {
            spent = GetTickCount() - tick;
            ret = joyGetPosEx(joyid, &infoex.ex);
            if (ret == JOYERR_NOERROR)
            {
                trace("X: %5d, Y: %5d, Z: %5d, POV: %5d\n",
                       infoex.ex.dwXpos, infoex.ex.dwYpos, infoex.ex.dwZpos, infoex.ex.dwPOV);
                trace("R: %5d, U: %5d, V: %5d\n",
                       infoex.ex.dwRpos, infoex.ex.dwUpos, infoex.ex.dwVpos);
                trace("BUTTONS: 0x%04X, BUTTON_COUNT: %2d, REMAINING: %d ms\n\n",
                       infoex.ex.dwButtons, infoex.ex.dwButtonNumber, MAX_TIME - spent);
            }
            Sleep(500);
        }
        while (spent < MAX_TIME);
#undef MAX_TIME
    }
    else
        skip("Skipping interactive tests for the joystick\n");
}

START_TEST(joystick)
{
    create_window();
    test_api();
    destroy_window();
}
