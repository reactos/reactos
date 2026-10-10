/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Common data
 * COPYRIGHT:   Copyright 2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/*
 * References:
 * https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
 * https://bsakatu.net/doc/virtual-key-of-windows/
 */
static const PCSTR VkNames[] =
{
/* 0x00 */
    "VK_NONE",
    "VK_LBUTTON",
    "VK_RBUTTON",
    "VK_CANCEL",
    "VK_MBUTTON",
// #if (_WIN32_WINNT >= 0x0500)
    "VK_XBUTTON1",
    "VK_XBUTTON2",
// #endif /* _WIN32_WINNT >= 0x0500 */
    NULL,               // Reserved
    "VK_BACK",
    "VK_TAB",
    NULL,               // Reserved
    NULL,               // Reserved
    "VK_CLEAR",
    "VK_RETURN",
    NULL,               // Unassigned
    NULL,               // Unassigned
/* 0x10 */
    "VK_SHIFT",
    "VK_CONTROL",
    "VK_MENU",
    "VK_PAUSE",
    "VK_CAPITAL",
    "VK_KANA-VK_HANGUL",
    "VK_IME_ON",
    "VK_JUNJA",
    "VK_FINAL",
    "VK_HANJA-VK_KANJI",
    "VK_IME_OFF",
    "VK_ESCAPE",
    "VK_CONVERT",
    "VK_NONCONVERT",
    "VK_ACCEPT",
    "VK_MODECHANGE",
/* 0x20 */
    "VK_SPACE",
    "VK_PRIOR",
    "VK_NEXT",
    "VK_END",
    "VK_HOME",
    "VK_LEFT",
    "VK_UP",
    "VK_RIGHT",
    "VK_DOWN",
    "VK_SELECT",
    "VK_PRINT",
    "VK_EXECUTE",
    "VK_SNAPSHOT",
    "VK_INSERT",
    "VK_DELETE",
    "VK_HELP",
/* 0x30 */
    /* VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39) */
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    NULL,               // Undefined
    NULL,               // Undefined
    NULL,               // Undefined
    NULL,               // Undefined
    NULL,               // Undefined
    NULL,               // Undefined
/* 0x40 */
    NULL,               // Undefined
    /* VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A) */
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
/* 0x50 */
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "VK_LWIN",
    "VK_RWIN",
    "VK_APPS",
    NULL,               // Reserved
    "VK_SLEEP",
/* 0x60 */
    "VK_NUMPAD0",
    "VK_NUMPAD1",
    "VK_NUMPAD2",
    "VK_NUMPAD3",
    "VK_NUMPAD4",
    "VK_NUMPAD5",
    "VK_NUMPAD6",
    "VK_NUMPAD7",
    "VK_NUMPAD8",
    "VK_NUMPAD9",
    "VK_MULTIPLY",
    "VK_ADD",
    "VK_SEPARATOR",
    "VK_SUBTRACT",
    "VK_DECIMAL",
    "VK_DIVIDE",
/* 0x70 */
    "VK_F1",
    "VK_F2",
    "VK_F3",
    "VK_F4",
    "VK_F5",
    "VK_F6",
    "VK_F7",
    "VK_F8",
    "VK_F9",
    "VK_F10",
    "VK_F11",
    "VK_F12",
    "VK_F13",
    "VK_F14",
    "VK_F15",
    "VK_F16",
/* 0x80 */
    "VK_F17",
    "VK_F18",
    "VK_F19",
    "VK_F20",
    "VK_F21",
    "VK_F22",
    "VK_F23",
    "VK_F24",
// #if (_WIN32_WINNT >= 0x0604)
    "VK_NAVIGATION_VIEW",   // Reserved
    "VK_NAVIGATION_MENU",   // Reserved
    "VK_NAVIGATION_UP",     // Reserved
    "VK_NAVIGATION_DOWN",   // Reserved
    "VK_NAVIGATION_LEFT",   // Reserved
    "VK_NAVIGATION_RIGHT",  // Reserved
    "VK_NAVIGATION_ACCEPT", // Reserved
    "VK_NAVIGATION_CANCEL", // Reserved
// #endif /* _WIN32_WINNT >= 0x0604 */
/* 0x90 */
    "VK_NUMLOCK",
    "VK_SCROLL",
    /*
     * OEM specific
     */
    /* NEC PC-9800 kbd definitions */
    // "VK_OEM_NEC_EQUAL",     // '=' key on numpad // Same value as VK_OEM_FJ_JISHO
    /*
     * Fujitsu/OASYS kbd definitions
     */
    "VK_OEM_FJ_JISHO",      // 'Dictionary' key
    "VK_OEM_FJ_MASSHOU",    // 'Unregister word' key
    "VK_OEM_FJ_TOUROKU",    // 'Register word' key
    "VK_OEM_FJ_LOYA",       // 'Left OYAYUBI' key
    "VK_OEM_FJ_ROYA",       // 'Right OYAYUBI' key
    NULL,               // Unassigned
    NULL,               // Unassigned
    NULL,               // Unassigned
    NULL,               // Unassigned
    NULL,               // Unassigned
    NULL,               // Unassigned
    NULL,               // Unassigned
    NULL,               // Unassigned
    NULL,               // Unassigned
/* 0xA0 */
    "VK_LSHIFT",
    "VK_RSHIFT",
    "VK_LCONTROL",
    "VK_RCONTROL",
    "VK_LMENU",
    "VK_RMENU",
// #if (_WIN32_WINNT >= 0x0500)
    "VK_BROWSER_BACK",
    "VK_BROWSER_FORWARD",
    "VK_BROWSER_REFRESH",
    "VK_BROWSER_STOP",
    "VK_BROWSER_SEARCH",
    "VK_BROWSER_FAVORITES",
    "VK_BROWSER_HOME",
    "VK_VOLUME_MUTE",
    "VK_VOLUME_DOWN",
    "VK_VOLUME_UP",
/* 0xB0 */
    "VK_MEDIA_NEXT_TRACK",
    "VK_MEDIA_PREV_TRACK",
    "VK_MEDIA_STOP",
    "VK_MEDIA_PLAY_PAUSE",
    "VK_LAUNCH_MAIL",
    "VK_LAUNCH_MEDIA_SELECT",
    "VK_LAUNCH_APP1",
    "VK_LAUNCH_APP2",
// #endif /* _WIN32_WINNT >= 0x0500 */
    NULL,               // Reserved
    NULL,               // Reserved
    "VK_OEM_1",         // ';:' for US
    "VK_OEM_PLUS",      // '+' any country
    "VK_OEM_COMMA",     // ',' any country
    "VK_OEM_MINUS",     // '-' any country
    "VK_OEM_PERIOD",    // '.' any country
    "VK_OEM_2",         // '/?' for US
/* 0xC0 */
    "VK_OEM_3",         // '`~' for US
    NULL,               // Reserved
    NULL,               // Reserved
// #if (_WIN32_WINNT >= 0x0604)
    "VK_GAMEPAD_A",
    "VK_GAMEPAD_B",
    "VK_GAMEPAD_X",
    "VK_GAMEPAD_Y",
    "VK_GAMEPAD_RIGHT_SHOULDER",
    "VK_GAMEPAD_LEFT_SHOULDER",
    "VK_GAMEPAD_LEFT_TRIGGER",
    "VK_GAMEPAD_RIGHT_TRIGGER",
    "VK_GAMEPAD_DPAD_UP",
    "VK_GAMEPAD_DPAD_DOWN",
    "VK_GAMEPAD_DPAD_LEFT",
    "VK_GAMEPAD_DPAD_RIGHT",
    "VK_GAMEPAD_MENU",
/* 0xD0 */
    "VK_GAMEPAD_VIEW",
    "VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON",
    "VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON",
    "VK_GAMEPAD_LEFT_THUMBSTICK_UP",
    "VK_GAMEPAD_LEFT_THUMBSTICK_DOWN",
    "VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT",
    "VK_GAMEPAD_LEFT_THUMBSTICK_LEFT",
    "VK_GAMEPAD_RIGHT_THUMBSTICK_UP",
    "VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN",
    "VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT",
    "VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT",
// #endif /* _WIN32_WINNT >= 0x0604 */
    "VK_OEM_4",         // '[{' for US
    "VK_OEM_5",         // '\|' for US
    "VK_OEM_6",         // ']}' for US
    "VK_OEM_7",         // ''"' for US
    "VK_OEM_8",
/* 0xE0 */
    NULL,               // Reserved
    "VK_OEM_AX",        // 'AX' key on Japanese AX kbd      // OEM specific
    "VK_OEM_102",       // "<>" or "\|" on RT 102-key kbd   // OEM specific
    "VK_ICO_HELP",      // Help key on ICO  // OEM specific
    "VK_ICO_00",        // 00 key on ICO    // OEM specific
// #if (WINVER >= 0x0400)
    "VK_PROCESSKEY",
// #endif /* WINVER >= 0x0400 */
    "VK_ICO_CLEAR",     // OEM specific
// #if (_WIN32_WINNT >= 0x0500)
    "VK_PACKET",
// #endif /* _WIN32_WINNT >= 0x0500 */
    NULL,               // Unassigned
    /*
     * OEM specific
     * Nokia/Ericsson definitions
     */
    "VK_OEM_RESET",
    "VK_OEM_JUMP",
    "VK_OEM_PA1",
    "VK_OEM_PA2",
    "VK_OEM_PA3",
    "VK_OEM_WSCTRL",
    "VK_OEM_CUSEL",
/* 0xF0 */
    "VK_OEM_ATTN",
    "VK_OEM_FINISH",
    "VK_OEM_COPY",
    "VK_OEM_AUTO",
    "VK_OEM_ENLW",
    "VK_OEM_BACKTAB",
    /* END of OEM definitions */
    "VK_ATTN",
    "VK_CRSEL",
    "VK_EXSEL",
    "VK_EREOF",
    "VK_PLAY",
    "VK_ZOOM",
    "VK_NONAME",        // Reserved
    "VK_PA1",
    "VK_OEM_CLEAR",
    "VK_UNKNOWN"
};
C_ASSERT(_countof(VkNames) == 0x100);

/* EOF */
