/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for RegisterHotKey
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#include <winuser.h>
#include <shlobj.h>
#include <undocshell.h>
#include <undocuser.h>

#define msg_hotkey(msg, id, mod, vk) do                                                             \
    {                                                                                               \
        ok((msg)->message == WM_HOTKEY, "Unexpected message %u\n", (msg)->message);                 \
        ok((msg)->hwnd == NULL, "hwnd = %p\n", (msg)->hwnd);                                        \
        ok((msg)->wParam == (id), "wParam = 0x%Ix\n", (msg)->wParam);                               \
        ok((msg)->lParam == MAKELONG(mod, vk),                                                      \
           "wParam = 0x%Ix, expected 0x%lx\n", (msg)->lParam, MAKELONG(mod, vk));                   \
    } while (0)
#define expect_hotkey(id, mod, vk) do                                                               \
    {                                                                                               \
        MSG msg;                                                                                    \
        int hotkey_count = 0;                                                                       \
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))                                               \
        {                                                                                           \
            msg_hotkey(&msg, id, mod, vk);                                                          \
            if (msg.message == WM_HOTKEY) hotkey_count++;                                           \
            DispatchMessageW(&msg);                                                                 \
        }                                                                                           \
        ok(hotkey_count == 1, "Received %d WM_HOTKEY messages, expected 1\n");                      \
    } while (0)
#define msg_no_hotkey(msg) do                                                                       \
    {                                                                                               \
        if ((msg)->message == WM_HOTKEY)                                                            \
            ok((msg)->message != WM_HOTKEY,                                                         \
               "Got WM_HOTKEY with hwnd=%p, wParam=0x%Ix, lParam=0x%Ix\n",                          \
               (msg)->hwnd, (msg)->wParam, (msg)->lParam);                                          \
        else                                                                                        \
            ok(0,                                                                                   \
               "Unexpected message %u posted to thread with hwnd=%p, wParam=0x%Ix, lParam=0x%Ix\n", \
               (msg)->message, (msg)->hwnd, (msg)->wParam, (msg)->lParam);                          \
    } while (0)
#define expect_no_hotkey() do                                                                       \
    {                                                                                               \
        MSG msg;                                                                                    \
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))                                               \
        {                                                                                           \
            msg_no_hotkey(&msg);                                                                    \
            DispatchMessageW(&msg);                                                                 \
        }                                                                                           \
    } while (0)

START_TEST(RegisterHotKey)
{
    SetCursorPos(0, 0);

    RegisterHotKey(NULL, 1, MOD_CONTROL, 0);
    RegisterHotKey(NULL, 2, MOD_CONTROL, 'U');
    RegisterHotKey(NULL, 3, MOD_CONTROL | MOD_ALT, 0);
    RegisterHotKey(NULL, 4, MOD_CONTROL | MOD_ALT, 'U');

    expect_no_hotkey();

    trace("Ctrl only\n");
    keybd_event(VK_CONTROL, 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        expect_hotkey(1, 0, VK_CONTROL);

    trace("Ctrl+U\n");
    keybd_event(VK_CONTROL, 0, 0, 0);
        expect_no_hotkey();
    keybd_event('U', 0, 0, 0);
        expect_hotkey(2, MOD_CONTROL, 'U');
    keybd_event('U', 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();

    trace("Ctrl+U (with Ctrl up first)\n");
    keybd_event(VK_CONTROL, 0, 0, 0);
        expect_no_hotkey();
    keybd_event('U', 0, 0, 0);
        expect_hotkey(2, MOD_CONTROL, 'U');
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();
    keybd_event('U', 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();

    trace("Ctrl+U (with U down first and Ctrl up first)\n");
    keybd_event('U', 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        expect_hotkey(1, 0, VK_CONTROL);
    keybd_event('U', 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();

    trace("Ctrl+U (with U down first and U up first)\n");
    keybd_event('U', 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, 0, 0);
        expect_no_hotkey();
    keybd_event('U', 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();

    trace("Ctrl+Alt\n");
    keybd_event(VK_CONTROL, 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_MENU, 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
        expect_hotkey(3, MOD_CONTROL, VK_MENU);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();

    trace("Ctrl+Alt (with Ctrl up first)\n");
    keybd_event(VK_CONTROL, 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_MENU, 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        expect_hotkey(3, MOD_ALT, VK_CONTROL);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();

    trace("Alt+Ctrl\n");
    keybd_event(VK_MENU, 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        expect_hotkey(3, MOD_ALT, VK_CONTROL);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();

    trace("Alt+Ctrl (with Alt up first)\n");
    keybd_event(VK_MENU, 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
        expect_hotkey(3, MOD_CONTROL, VK_MENU);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();

    trace("Alt+U\n");
    keybd_event(VK_MENU, 0, 0, 0);
        expect_no_hotkey();
    keybd_event('U', 0, 0, 0);
        expect_no_hotkey();
    keybd_event('U', 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();

    trace("Ctrl+Alt+U\n");
    keybd_event(VK_CONTROL, 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_MENU, 0, 0, 0);
        expect_no_hotkey();
    keybd_event('U', 0, 0, 0);
        expect_hotkey(4, MOD_CONTROL | MOD_ALT, 'U');
    keybd_event('U', 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();

    trace("Alt+Ctrl+U\n");
    keybd_event(VK_MENU, 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, 0, 0);
        expect_no_hotkey();
    keybd_event('U', 0, 0, 0);
        expect_hotkey(4, MOD_CONTROL | MOD_ALT, 'U');
    keybd_event('U', 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();

    trace("Ctrl+U+Alt\n");
    keybd_event(VK_CONTROL, 0, 0, 0);
        expect_no_hotkey();
    keybd_event('U', 0, 0, 0);
        expect_hotkey(2, MOD_CONTROL, 'U');
    keybd_event(VK_MENU, 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
        expect_hotkey(3, MOD_CONTROL, VK_MENU);
    keybd_event('U', 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();

    trace("Alt+U+Ctrl\n");
    keybd_event(VK_MENU, 0, 0, 0);
        expect_no_hotkey();
    keybd_event('U', 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, 0, 0);
        expect_no_hotkey();
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        expect_hotkey(3, MOD_ALT, VK_CONTROL);
    keybd_event('U', 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
        expect_no_hotkey();

    /* The remaining combinations are an exercise left to the reader */

    UnregisterHotKey(NULL, 4);
    UnregisterHotKey(NULL, 3);
    UnregisterHotKey(NULL, 2);
    UnregisterHotKey(NULL, 1);

    expect_no_hotkey();
}
