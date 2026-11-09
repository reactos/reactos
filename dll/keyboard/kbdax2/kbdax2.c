/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Japanese Keyboard Layout for AX2
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
/* See also: https://kbdlayout.info/kbdax2 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

#ifdef _M_IA64
  #define ROSDATA static __declspec(allocate(".data"))
#else
  #ifdef _MSC_VER
    #pragma data_seg(".data")
    #define ROSDATA static
  #else
    #define ROSDATA static __attribute__((section(".data")))
  #endif
#endif

#define VK_EMPTY 0xff
#define KNUMS    (KBDNUMPAD | KBDSPECIAL)
#define KMEXT    (KBDEXT | KBDMULTIVK)

/* Modifier bit for the Kana state (not in every ndk/kbd.h) */
#ifndef KBDKANA
  #define KBDKANA 0x08
#endif

#ifndef VK_OEM_AX
  #define VK_OEM_AX 0xE1
#endif

/* ---- scancode -> VK (bMaxVSCtoVK = 0x80) ---------------------------- */
ROSDATA USHORT scancode_to_vk[0x80] = {
  /*00*/ VK_EMPTY, VK_ESCAPE, '1', '2',
  /*04*/ '3', '4', '5', '6',
  /*08*/ '7', '8', '9', '0',
  /*0c*/ VK_OEM_MINUS, VK_OEM_PLUS, VK_BACK, VK_TAB,
  /*10*/ 'Q', 'W', 'E', 'R',
  /*14*/ 'T', 'Y', 'U', 'I',
  /*18*/ 'O', 'P', VK_OEM_4, VK_OEM_6,
  /*1c*/ VK_RETURN, VK_LCONTROL, 'A', 'S',
  /*20*/ 'D', 'F', 'G', 'H',
  /*24*/ 'J', 'K', 'L', VK_OEM_1,
  /*28*/ VK_OEM_7, VK_OEM_3, VK_LSHIFT, VK_OEM_5,
  /*2c*/ 'Z', 'X', 'C', 'V',
  /*30*/ 'B', 'N', 'M', VK_OEM_COMMA,
  /*34*/ VK_OEM_PERIOD, VK_OEM_2, VK_RSHIFT | KBDEXT, VK_MULTIPLY | KBDMULTIVK,
  /*38*/ VK_LMENU, VK_SPACE, VK_CAPITAL, VK_F1,
  /*3c*/ VK_F2, VK_F3, VK_F4, VK_F5,
  /*40*/ VK_F6, VK_F7, VK_F8, VK_F9,
  /*44*/ VK_F10, VK_NUMLOCK | KMEXT, VK_SCROLL | KBDMULTIVK, VK_HOME | KNUMS,
  /*48*/ VK_UP | KNUMS, VK_PRIOR | KNUMS, VK_SUBTRACT, VK_LEFT | KNUMS,
  /*4c*/ VK_CLEAR | KNUMS, VK_RIGHT | KNUMS, VK_ADD, VK_END | KNUMS,
  /*50*/ VK_DOWN | KNUMS, VK_NEXT | KNUMS, VK_INSERT | KNUMS, VK_DELETE | KNUMS,
  /*54*/ VK_SNAPSHOT, VK_EMPTY, VK_OEM_102, VK_F11,
  /*58*/ VK_F12, VK_CLEAR, VK_NONCONVERT | KBDSPECIAL, VK_CONVERT | KBDSPECIAL,
  /*5c*/ VK_OEM_AX, VK_EREOF, VK_EMPTY, VK_NONAME,
  /*60*/ VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  /*64*/ VK_F13, VK_F14, VK_F15, VK_F16,
  /*68*/ VK_F17, VK_F18, VK_F19, VK_F20,
  /*6c*/ VK_F21, VK_F22, VK_F23, VK_EMPTY,
  /*70*/ VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  /*74*/ VK_EMPTY, VK_EMPTY, VK_F24, VK_EMPTY,
  /*78*/ VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  /*7c*/ VK_TAB, VK_EMPTY, VK_ABNT_C2, VK_OEM_PA2
};

ROSDATA VSC_VK extcode0_to_vk[] = {
  { 0x1c, VK_RETURN | KBDEXT },
  { 0x1d, VK_OEM_FINISH | KBDEXT | KBDSPECIAL },
  { 0x35, VK_DIVIDE | KBDEXT },
  { 0x37, VK_SNAPSHOT | KBDEXT },
  { 0x38, VK_KANJI | KBDEXT | KBDSPECIAL },
  { 0x46, VK_CANCEL | KBDEXT },
  { 0x47, VK_HOME | KBDEXT },
  { 0x48, VK_UP | KBDEXT },
  { 0x49, VK_PRIOR | KBDEXT },
  { 0x4b, VK_LEFT | KBDEXT },
  { 0x4d, VK_RIGHT | KBDEXT },
  { 0x4f, VK_END | KBDEXT },
  { 0x50, VK_DOWN | KBDEXT },
  { 0x51, VK_NEXT | KBDEXT },
  { 0x52, VK_INSERT | KBDEXT },
  { 0x53, VK_DELETE | KBDEXT },
  { 0x5b, VK_LWIN | KBDEXT },
  { 0x5c, VK_RWIN | KBDEXT },
  { 0x5d, VK_APPS | KBDEXT },
  { 0, 0 }
};

ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0x1d, VK_PAUSE },
  { 0, 0 }
};

/* ---- Modifiers -------------------------------------- */
ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT,   KBDSHIFT },
  { VK_CONTROL, KBDCTRL },
  { VK_MENU,    KBDALT },
  { VK_KANA,    KBDKANA },
  { 0, 0 }
};

/*
 * Modifier bits -> column number. Bit 0: Shift, bit 1: Ctrl, bit 2: Alt, bit 3: Kana.
 * Columns: 0 = base, 1 = Shift, 2 = Kana, 3 = Kana+Shift, 4 = Ctrl, 5 = Kana+Ctrl,
 *          6 = Shift+Ctrl, 7 = Kana+Shift+Ctrl. Alt combinations are invalid.
 */
ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  11,
  {
    0,            /* 0000: none */
    1,            /* 0001: Shift */
    4,            /* 0010: Ctrl */
    6,            /* 0011: Shift+Ctrl */
    SHFT_INVALID, /* 0100: Alt */
    SHFT_INVALID, /* 0101: Alt+Shift */
    SHFT_INVALID, /* 0110: Alt+Ctrl */
    SHFT_INVALID, /* 0111: Alt+Shift+Ctrl */
    2,            /* 1000: Kana */
    3,            /* 1001: Kana+Shift */
    5,            /* 1010: Kana+Ctrl */
    7             /* 1011: Kana+Shift+Ctrl */
  }
};

/* ---- VK -> WCHAR ---------------------------------------------------- */
ROSDATA VK_TO_WCHARS6 key_to_chars_6mod[] = {
  { VK_BACK, 0, { 0x08, 0x08, 0x08, 0x08, 0x7f, 0x7f } },
  { VK_CANCEL, 0, { 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 } },
  { VK_ESCAPE, 0, { 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b } },
  { VK_OEM_4, KANALOK, { '[', '{', 0xff9e, 0xff9e, 0x1b, 0x1b } },
  { VK_OEM_5, KANALOK, { '\\', '|', 0xff70, 0xff70, 0x1c, 0x1c } },
  { VK_OEM_102, KANALOK, { '\\', '|', 0xff9b, 0xff9b, 0x1c, 0x1c } },
  { VK_OEM_6, KANALOK, { ']', '}', 0xff9f, 0xff62, 0x1d, 0x1d } },
  { VK_RETURN, 0, { '\r', '\r', '\r', '\r', '\n', '\n' } },
  { VK_SPACE, 0, { ' ', ' ', ' ', ' ', ' ', ' ' } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS8 key_to_chars_8mod[] = {
  { '2', KANALOK, { '2', '@', 0xff8c, 0xff8c, WCH_NONE, WCH_NONE, 0x00, 0x00 } },
  { '6', KANALOK, { '6', '^', 0xff75, 0xff6b, WCH_NONE, WCH_NONE, 0x1e, 0x1e } },
  { VK_OEM_MINUS, KANALOK, { '-', '_', 0xff8e, 0xff8e, WCH_NONE, WCH_NONE, 0x1f, 0x1f } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { '0', KANALOK, { '0', ')', 0xff9c, 0xff66 } },
  { '1', KANALOK, { '1', '!', 0xff87, 0xff87 } },
  { '3', KANALOK, { '3', '#', 0xff71, 0xff67 } },
  { '4', KANALOK, { '4', '$', 0xff73, 0xff69 } },
  { '5', KANALOK, { '5', '%', 0xff74, 0xff6a } },
  { '7', KANALOK, { '7', '&', 0xff94, 0xff6c } },
  { '8', KANALOK, { '8', '*', 0xff95, 0xff6d } },
  { '9', KANALOK, { '9', '(', 0xff96, 0xff6e } },
  { 'A', CAPLOK|KANALOK, { 'a', 'A', 0xff81, 0xff81 } },
  { 'B', CAPLOK|KANALOK, { 'b', 'B', 0xff7a, 0xff7a } },
  { 'C', CAPLOK|KANALOK, { 'c', 'C', 0xff7f, 0xff7f } },
  { 'D', CAPLOK|KANALOK, { 'd', 'D', 0xff7c, 0xff7c } },
  { 'E', CAPLOK|KANALOK, { 'e', 'E', 0xff72, 0xff68 } },
  { 'F', CAPLOK|KANALOK, { 'f', 'F', 0xff8a, 0xff8a } },
  { 'G', CAPLOK|KANALOK, { 'g', 'G', 0xff77, 0xff77 } },
  { 'H', CAPLOK|KANALOK, { 'h', 'H', 0xff78, 0xff78 } },
  { 'I', CAPLOK|KANALOK, { 'i', 'I', 0xff86, 0xff86 } },
  { 'J', CAPLOK|KANALOK, { 'j', 'J', 0xff8f, 0xff8f } },
  { 'K', CAPLOK|KANALOK, { 'k', 'K', 0xff89, 0xff89 } },
  { 'L', CAPLOK|KANALOK, { 'l', 'L', 0xff98, 0xff98 } },
  { 'M', CAPLOK|KANALOK, { 'm', 'M', 0xff93, 0xff93 } },
  { 'N', CAPLOK|KANALOK, { 'n', 'N', 0xff90, 0xff90 } },
  { 'O', CAPLOK|KANALOK, { 'o', 'O', 0xff97, 0xff97 } },
  { 'P', CAPLOK|KANALOK, { 'p', 'P', 0xff7e, 0xff7e } },
  { 'Q', CAPLOK|KANALOK, { 'q', 'Q', 0xff80, 0xff80 } },
  { 'R', CAPLOK|KANALOK, { 'r', 'R', 0xff7d, 0xff7d } },
  { 'S', CAPLOK|KANALOK, { 's', 'S', 0xff84, 0xff84 } },
  { 'T', CAPLOK|KANALOK, { 't', 'T', 0xff76, 0xff76 } },
  { 'U', CAPLOK|KANALOK, { 'u', 'U', 0xff85, 0xff85 } },
  { 'V', CAPLOK|KANALOK, { 'v', 'V', 0xff8b, 0xff8b } },
  { 'W', CAPLOK|KANALOK, { 'w', 'W', 0xff83, 0xff83 } },
  { 'X', CAPLOK|KANALOK, { 'x', 'X', 0xff7b, 0xff7b } },
  { 'Y', CAPLOK|KANALOK, { 'y', 'Y', 0xff9d, 0xff9d } },
  { 'Z', CAPLOK|KANALOK, { 'z', 'Z', 0xff82, 0xff6f } },
  { VK_OEM_1, KANALOK, { ';', ':', 0xff9a, 0xff9a } },
  { VK_OEM_2, KANALOK, { '/', '?', 0xff92, 0xff65 } },
  { VK_OEM_3, KANALOK, { '`', '~', 0xff91, 0xff63 } },
  { VK_OEM_7, KANALOK, { '\'', '"', 0xff79, 0xff79 } },
  { VK_OEM_8, 0, { WCH_NONE, WCH_NONE, WCH_NONE, WCH_NONE } },
  { VK_OEM_COMMA, KANALOK, { ',', '<', 0xff88, 0xff64 } },
  { VK_OEM_PERIOD, KANALOK, { '.', '>', 0xff99, 0xff61 } },
  { VK_OEM_PLUS, KANALOK, { '=', '+', 0xff8d, 0xff8d } },
  { VK_TAB, 0, { '\t', '\t', '\t', '\t' } },
  { VK_ADD, 0, { '+', '+', '+', '+' } },
  { VK_DECIMAL, 0, { '.', '.', '.', '.' } },
  { VK_DIVIDE, 0, { '/', '/', '/', '/' } },
  { VK_MULTIPLY, 0, { '*', '*', '*', '*' } },
  { VK_SUBTRACT, 0, { '-', '-', '-', '-' } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 keypad_numbers[] = {
  { VK_NUMPAD0, 0, { '0', WCH_NONE, '0', WCH_NONE } },
  { VK_NUMPAD1, 0, { '1', WCH_NONE, '1', WCH_NONE } },
  { VK_NUMPAD2, 0, { '2', WCH_NONE, '2', WCH_NONE } },
  { VK_NUMPAD3, 0, { '3', WCH_NONE, '3', WCH_NONE } },
  { VK_NUMPAD4, 0, { '4', WCH_NONE, '4', WCH_NONE } },
  { VK_NUMPAD5, 0, { '5', WCH_NONE, '5', WCH_NONE } },
  { VK_NUMPAD6, 0, { '6', WCH_NONE, '6', WCH_NONE } },
  { VK_NUMPAD7, 0, { '7', WCH_NONE, '7', WCH_NONE } },
  { VK_NUMPAD8, 0, { '8', WCH_NONE, '8', WCH_NONE } },
  { VK_NUMPAD9, 0, { '9', WCH_NONE, '9', WCH_NONE } },
  { 0, 0 }
};

#define vk_master(n,x) { (PVK_TO_WCHARS1)x, n, sizeof(x[0]) }
ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(6, key_to_chars_6mod),
  vk_master(8, key_to_chars_8mod),
  vk_master(4, key_to_chars_4mod),
  vk_master(4, keypad_numbers),
  { 0, 0, 0 }
};

/* ---- Key names --------------------------------------------------------- */
ROSDATA VSC_LPWSTR key_names[] = {
  { 0x01, L"Esc" },
  { 0x0e, L"Backspace" },
  { 0x0f, L"Tab" },
  { 0x1c, L"Enter" },
  { 0x1d, L"Ctrl" },
  { 0x2a, L"Shift" },
  { 0x36, L"Right Shift" },
  { 0x37, L"Num *" },
  { 0x38, L"Alt" },
  { 0x39, L"Space" },
  { 0x3a, L"Caps Lock" },
  { 0x3b, L"F1" },
  { 0x3c, L"F2" },
  { 0x3d, L"F3" },
  { 0x3e, L"F4" },
  { 0x3f, L"F5" },
  { 0x40, L"F6" },
  { 0x41, L"F7" },
  { 0x42, L"F8" },
  { 0x43, L"F9" },
  { 0x44, L"F10" },
  { 0x45, L"Pause" },
  { 0x46, L"Scroll Lock" },
  { 0x47, L"Num 7" },
  { 0x48, L"Num 8" },
  { 0x49, L"Num 9" },
  { 0x4a, L"Num -" },
  { 0x4b, L"Num 4" },
  { 0x4c, L"Num 5" },
  { 0x4d, L"Num 6" },
  { 0x4e, L"Num +" },
  { 0x4f, L"Num 1" },
  { 0x50, L"Num 2" },
  { 0x51, L"Num 3" },
  { 0x52, L"Num 0" },
  { 0x53, L"Num Del" },
  { 0x54, L"Sys Req" },
  { 0x57, L"F11" },
  { 0x58, L"F12" },
  { 0x5a, L"\x7121" L"\x5909" L"\x63db" },  /* Muhenkan */
  { 0x5b, L"\x5909" L"\x63db" },  /* Henkan */
  { 0x5c, L"AX" },
  { 0x7c, L"F13" },
  { 0x7d, L"F14" },
  { 0x7e, L"F15" },
  { 0x7f, L"F16" },
  { 0x80, L"F17" },
  { 0x81, L"F18" },
  { 0x82, L"F19" },
  { 0x83, L"F20" },
  { 0x84, L"F21" },
  { 0x85, L"F22" },
  { 0x86, L"F23" },
  { 0x87, L"F24" },
  { 0, NULL }
};

ROSDATA VSC_LPWSTR extended_key_names[] = {
  { 0x1c, L"Num Enter" },
  { 0x1d, L"\x82f1" L"\x6570" L"\x0020" L"\x30ab" L"\x30ca" },  /* Eisuu Kana */
  { 0x35, L"Num /" },
  { 0x37, L"Prnt Scrn" },
  { 0x38, L"\x6f22" L"\x5b57" },  /* Kanji */
  { 0x45, L"Num Lock" },
  { 0x46, L"Break" },
  { 0x47, L"Home" },
  { 0x48, L"Up" },
  { 0x49, L"Page Up" },
  { 0x4b, L"Left" },
  { 0x4d, L"Right" },
  { 0x4f, L"End" },
  { 0x50, L"Down" },
  { 0x51, L"Page Down" },
  { 0x52, L"Insert" },
  { 0x53, L"Delete" },
  { 0x54, L"<00>" },
  { 0x56, L"Help" },
  { 0x5b, L"Left Windows" },
  { 0x5c, L"Right Windows" },
  { 0x5d, L"Application" },
  { 0, NULL }
};

/* ---- Master table ------------------------------------------------ */
ROSDATA KBDTABLES_FE keyboard_layout_table = {
  {
    &modifier_bits,
    vk_to_wchar_master_table,
    NULL,                       /* No dead keys */
    key_names,
    extended_key_names,
    NULL,
    scancode_to_vk,
    0x80,                       /* bMaxVSCtoVK */
    extcode0_to_vk,
    extcode1_to_vk,
    0,                          /* fLocaleFlags */
    0, 0, NULL                  /* No ligatures */
  },
  7,                            /* dwType    : Japanese */
  1                             /* dwSubType : AX2 */
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table.Base;
}

/* ---- NLS (Far-East) Table: KbdNlsLayerDescriptor ------------------ */
ROSDATA VK_F nls_vk_to_f[] = {
  { VK_OEM_FINISH,
    KBDNLS_TYPE_TOGGLE, KBDNLS_INDEX_NORMAL, 0x08,
    { /* NLSFEProc: modifier state 0..7 (bit0=Shift, bit1=Ctrl, bit2=Alt) */
      { KBDNLS_KATAKANA, 0 }, { KBDNLS_HIRAGANA, 0 },
      { KBDNLS_ROMAN, 0 }, { KBDNLS_SEND_PARAM_VK, VK_KANA },
      { KBDNLS_ALPHANUM, 0 }, { KBDNLS_ALPHANUM, 0 },
      { KBDNLS_ALPHANUM, 0 }, { KBDNLS_ALPHANUM, 0 }
    },
    { /* NLSFEProcAlt */
      { KBDNLS_SEND_PARAM_VK, VK_KANA }, { KBDNLS_SEND_PARAM_VK, VK_KANA },
      { KBDNLS_SEND_PARAM_VK, VK_KANA }, { KBDNLS_SEND_PARAM_VK, VK_KANA },
      { KBDNLS_NOEVENT, 0 }, { KBDNLS_NOEVENT, 0 },
      { KBDNLS_NOEVENT, 0 }, { KBDNLS_NOEVENT, 0 }
    }
  },
  { VK_NONCONVERT,
    KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0x00,
    { /* NLSFEProc: modifier state 0..7 (bit0=Shift, bit1=Ctrl, bit2=Alt) */
      { KBDNLS_SEND_BASE_VK, 0 }, { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERWORDREGISTERMODE },
      { KBDNLS_CODEINPUT, 0 }, { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERWORDREGISTERMODE },
      { KBDNLS_SEND_BASE_VK, 0 }, { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERWORDREGISTERMODE },
      { KBDNLS_CODEINPUT, 0 }, { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERWORDREGISTERMODE }
    },
    { { 0 } }
  },
  { VK_KANJI,
    KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0x00,
    { /* NLSFEProc: modifier state 0..7 (bit0=Shift, bit1=Ctrl, bit2=Alt) */
      { KBDNLS_SEND_BASE_VK, 0 }, { KBDNLS_SBCSDBCS, 0 },
      { KBDNLS_SEND_BASE_VK, 0 }, { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERIMECONFIGMODE },
      { KBDNLS_SEND_BASE_VK, 0 }, { KBDNLS_SBCSDBCS, 0 },
      { KBDNLS_SEND_BASE_VK, 0 }, { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERIMECONFIGMODE }
    },
    { { 0 } }
  }
};

ROSDATA KBDNLSTABLES nls_tables = {
  0,                         /* OEMIdentifier */
  0,                         /* LayoutInformation */
  3,                         /* NumOfVkToF */
  nls_vk_to_f,
  0,                         /* NumOfMouseVKey */
  NULL
};

PKBDNLSTABLES WINAPI KbdNlsLayerDescriptor(VOID) {
  return &nls_tables;
}
