/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for keyboard inputs with flags KEYEVENTF_EXTENDEDKEY and KEYEVENTF_SCANCODE
 * COPYRIGHT:   Copyright 2021 Arjav Garg <arjavgarg@gmail.com>
 */

#include "precomp.h"

static void testScancodeExtendedKey(BYTE wVk, BYTE scanCode)
{
    trace("wVK: %x\tScancode: %x\n", wVk, scanCode);

    keybd_event(0, scanCode, KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY, 0);
    SHORT winKeyState = GetAsyncKeyState(wVk);
    ok(winKeyState & 0x8000, "VK=%x should be detected as key down.\n", wVk);

    keybd_event(0, scanCode, KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    winKeyState = GetAsyncKeyState(wVk);
    ok(!(winKeyState & 0x8000), "VK=%x should be detected as key up.\n", wVk);
}

/* https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input#extended-key-flag */
START_TEST(keybd_event)
{
    testScancodeExtendedKey(VK_RWIN, 0x5C);
    testScancodeExtendedKey(VK_LWIN, 0x5B);
    testScancodeExtendedKey(VK_RMENU, 0x38);
    testScancodeExtendedKey(VK_RCONTROL, 0x1D);
    testScancodeExtendedKey(VK_INSERT, 0x52);
    testScancodeExtendedKey(VK_DELETE, 0x53);
    testScancodeExtendedKey(VK_HOME, 0x47);
    testScancodeExtendedKey(VK_END, 0x4f);
    testScancodeExtendedKey(VK_PRIOR, 0x49);
    testScancodeExtendedKey(VK_NEXT, 0x51);
    testScancodeExtendedKey(VK_UP, 0x48);
    testScancodeExtendedKey(VK_RIGHT, 0x4d);
    testScancodeExtendedKey(VK_LEFT, 0x4b);
    testScancodeExtendedKey(VK_DOWN, 0x50);
    testScancodeExtendedKey(VK_DIVIDE, 0x35);
    testScancodeExtendedKey(VK_RETURN, 0x1C);
}
