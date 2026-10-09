/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NEC PC98-NX Japanese keyboard layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

/* See also: https://kbdlayout.info/kbdnecat */

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

#define VK_EMPTY  0xff /* The non-existent VK */
#define KNUMS (KBDNUMPAD | KBDSPECIAL) /* Special + number pad */
#define KMEXT (KBDEXT | KBDMULTIVK)    /* Multi + ext */
#define KBDKANA   0x08
#define VK_OEM_NEC_EQUAL 0x92

/* ------------------------------------------------------------------ */
/* Scancode -> VK  (bMaxVSCtoVK = 0x80)                               */
/* ------------------------------------------------------------------ */
ROSDATA USHORT scancode_to_vk[] = {
  /* - 00 - */
  VK_EMPTY,     VK_ESCAPE,    '1',          '2',
  '3',          '4',          '5',          '6',
  '7',          '8',          '9',          '0',
  VK_OEM_MINUS,                  /* 0c: - = */
  VK_OEM_7,                      /* 0d: ^ ~ */
  VK_BACK,
  /* - 0f - */
  VK_TAB,       'Q',          'W',          'E',
  'R',          'T',          'Y',          'U',
  'I',          'O',          'P',
  VK_OEM_3,                      /* 1a: @ ` */
  VK_OEM_4,                      /* 1b: [ { */
  VK_RETURN,
  /* - 1d - */
  VK_LCONTROL,
  'A',          'S',          'D',          'F',
  'G',          'H',          'J',          'K',
  'L',
  VK_OEM_PLUS,                   /* 27: ; + */
  VK_OEM_1,                      /* 28: : * */
  VK_DBE_SBCSCHAR | KBDSPECIAL,  /* 29: Hankaku/Zenkaku */
  VK_LSHIFT,
  VK_OEM_6,                      /* 2b: ] } */
  /* - 2c - */
  'Z',          'X',          'C',          'V',
  'B',          'N',          'M',          VK_OEM_COMMA,
  VK_OEM_PERIOD,VK_OEM_2,
  VK_RSHIFT | KBDEXT,
  /* - 37 - */
  VK_MULTIPLY,  VK_LMENU,     VK_SPACE,     VK_CAPITAL,
  /* - 3b - */
  VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6,
  VK_F7, VK_F8, VK_F9, VK_F10,
  /* - 45 - */
  VK_NUMLOCK | KMEXT,
  VK_SCROLL,
  /* - 47 - */
  VK_HOME | KNUMS,      VK_UP | KNUMS,         VK_PRIOR | KNUMS, VK_SUBTRACT,
  VK_LEFT | KNUMS,      VK_CLEAR | KNUMS,      VK_RIGHT | KNUMS, VK_ADD,
  VK_END | KNUMS,       VK_DOWN | KNUMS,       VK_NEXT | KNUMS,
  VK_INSERT | KNUMS,    VK_DELETE | KNUMS,
  /* - 54 - */
  VK_SNAPSHOT,
  /* - 55 - */
  VK_EMPTY,     VK_EMPTY,     VK_F11,       VK_F12,
  /* - 59 - */
  VK_OEM_NEC_EQUAL | KBDSPECIAL,  /* 59: Ten-key = */
  VK_NONAME,                      /* 5a */
  VK_EMPTY,
  VK_SEPARATOR,                   /* 5c: Ten-key , */
  VK_F13,       VK_F14,       VK_F15,
  /* - 60 - */
  VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  /* - 70 - */
  VK_KANA,                        /* 70: Kana */
  VK_EMPTY,     VK_EMPTY,
  VK_OEM_8,                       /* 73 */
  VK_OEM_NEC_EQUAL,               /* 74 */
  VK_SEPARATOR,                   /* 75 */
  VK_EMPTY,     VK_EMPTY,     VK_EMPTY,
  VK_CONVERT | KBDSPECIAL,        /* 79: XFER */
  VK_EMPTY,
  VK_NONCONVERT | KBDSPECIAL,     /* 7b: NFER */
  VK_EMPTY,
  VK_OEM_5,                       /* 7d: \ | (￥) */
  VK_ABNT_C2,                     /* 7e */
  0xec,                           /* 7f */
};

ROSDATA VSC_VK extcode0_to_vk[] = {
  { 0x1C, VK_RETURN | KBDEXT },
  { 0x1D, VK_RCONTROL | KBDEXT },
  { 0x35, VK_DIVIDE | KBDEXT },
  { 0x37, VK_SNAPSHOT | KBDEXT },           /* COPY */
  { 0x38, VK_EMPTY | KBDEXT },              /* Right-GRPH */
  { 0x46, VK_CANCEL | KBDEXT },             /* STOP */
  { 0x47, VK_HOME | KBDEXT | KBDSPECIAL },  /* CLR */
  { 0x48, VK_UP | KBDEXT },
  { 0x49, VK_PRIOR | KBDEXT },              /* ROLL DOWN */
  { 0x4B, VK_LEFT | KBDEXT },
  { 0x4D, VK_RIGHT | KBDEXT },
  { 0x4F, VK_END | KBDEXT | KBDSPECIAL },   /* HELP */
  { 0x50, VK_DOWN | KBDEXT },
  { 0x51, VK_NEXT | KBDEXT },               /* ROLL UP */
  { 0x52, VK_INSERT | KBDEXT },
  { 0x53, VK_DELETE | KBDEXT },
  { 0x5B, VK_LWIN | KBDEXT },
  { 0x5C, VK_RWIN | KBDEXT },
  { 0x5D, VK_APPS | KBDEXT },
  { 0, 0 },
};

ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0x1d, VK_PAUSE },
  { 0, 0 },
};

/* ------------------------------------------------------------------ */
/* Modifiers  (Shift / Ctrl / Alt / Kana)                             */
/* ------------------------------------------------------------------ */
ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT,   KBDSHIFT },
  { VK_CONTROL, KBDCTRL },
  { VK_MENU,    KBDALT },
  { VK_KANA,    KBDKANA },
  { 0,          0 }
};

ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  11,
  {
      0,            /* NONE */
      1,            /* SHIFT */
      4,            /* CTRL */
      6,            /* SHIFT+CTRL */
      SHFT_INVALID, /* ALT */
      SHFT_INVALID, /* SHIFT+ALT */
      SHFT_INVALID, /* CTRL+ALT */
      SHFT_INVALID, /* SHIFT+CTRL+ALT */
      2,            /* KANA */
      3,            /* KANA+SHIFT */
      5,            /* KANA+CTRL */
      7,            /* KANA+SHIFT+CTRL */
  }
};

/* ------------------------------------------------------------------ */
/* VK -> WCHAR tables                                                 */
/* ------------------------------------------------------------------ */

/* Normal / Shift / Kana / Kana+Shift */
ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { '0',  KANALOK, {'0', '0', 0xff9c, 0xff66} },           /* ﾜ ｦ */
  { '1',  KANALOK, {'1', '!', 0xff87, 0xff87} },           /* ﾇ */
  { '3',  KANALOK, {'3', '#', 0xff71, 0xff67} },           /* ｱ ｧ */
  { '4',  KANALOK, {'4', '$', 0xff73, 0xff69} },           /* ｳ ｩ */
  { '5',  KANALOK, {'5', '%', 0xff74, 0xff6a} },           /* ｴ ｪ */
  { '7',  KANALOK, {'7', '\'',0xff94, 0xff6c} },           /* ﾔ ｬ */
  { '8',  KANALOK, {'8', '(', 0xff95, 0xff6d} },           /* ﾕ ｭ */
  { '9',  KANALOK, {'9', ')', 0xff96, 0xff6e} },           /* ﾖ ｮ */
  { 'A',  KANALOK|CAPLOK, {'a', 'A', 0xff81, 0xff81} },    /* ﾁ */
  { 'B',  KANALOK|CAPLOK, {'b', 'B', 0xff7a, 0xff7a} },    /* ｺ */
  { 'C',  KANALOK|CAPLOK, {'c', 'C', 0xff7f, 0xff7f} },    /* ｿ */
  { 'D',  KANALOK|CAPLOK, {'d', 'D', 0xff7c, 0xff7c} },    /* ｼ */
  { 'E',  KANALOK|CAPLOK, {'e', 'E', 0xff72, 0xff68} },    /* ｲ ｨ */
  { 'F',  KANALOK|CAPLOK, {'f', 'F', 0xff8a, 0xff8a} },    /* ﾊ */
  { 'G',  KANALOK|CAPLOK, {'g', 'G', 0xff77, 0xff77} },    /* ｷ */
  { 'H',  KANALOK|CAPLOK, {'h', 'H', 0xff78, 0xff78} },    /* ｸ */
  { 'I',  KANALOK|CAPLOK, {'i', 'I', 0xff86, 0xff86} },    /* ﾆ */
  { 'J',  KANALOK|CAPLOK, {'j', 'J', 0xff8f, 0xff8f} },    /* ﾏ */
  { 'K',  KANALOK|CAPLOK, {'k', 'K', 0xff89, 0xff89} },    /* ﾉ */
  { 'L',  KANALOK|CAPLOK, {'l', 'L', 0xff98, 0xff98} },    /* ﾘ */
  { 'M',  KANALOK|CAPLOK, {'m', 'M', 0xff93, 0xff93} },    /* ﾓ */
  { 'N',  KANALOK|CAPLOK, {'n', 'N', 0xff90, 0xff90} },    /* ﾐ */
  { 'O',  KANALOK|CAPLOK, {'o', 'O', 0xff97, 0xff97} },    /* ﾗ */
  { 'P',  KANALOK|CAPLOK, {'p', 'P', 0xff7e, 0xff7e} },    /* ｾ */
  { 'Q',  KANALOK|CAPLOK, {'q', 'Q', 0xff80, 0xff80} },    /* ﾀ */
  { 'R',  KANALOK|CAPLOK, {'r', 'R', 0xff7d, 0xff7d} },    /* ｽ */
  { 'S',  KANALOK|CAPLOK, {'s', 'S', 0xff84, 0xff84} },    /* ﾄ */
  { 'T',  KANALOK|CAPLOK, {'t', 'T', 0xff76, 0xff76} },    /* ｶ */
  { 'U',  KANALOK|CAPLOK, {'u', 'U', 0xff85, 0xff85} },    /* ﾅ */
  { 'V',  KANALOK|CAPLOK, {'v', 'V', 0xff8b, 0xff8b} },    /* ﾋ */
  { 'W',  KANALOK|CAPLOK, {'w', 'W', 0xff83, 0xff83} },    /* ﾃ */
  { 'X',  KANALOK|CAPLOK, {'x', 'X', 0xff7b, 0xff7b} },    /* ｻ */
  { 'Y',  KANALOK|CAPLOK, {'y', 'Y', 0xff9d, 0xff9d} },    /* ﾝ */
  { 'Z',  KANALOK|CAPLOK, {'z', 'Z', 0xff82, 0xff6f} },    /* ﾂ ｯ */
  { VK_OEM_1,      KANALOK, {':', '*', 0xff79, 0xff79} },  /* ｹ  (0xBA) */
  { VK_OEM_2,      KANALOK, {'/', '?', 0xff92, 0xff65} },  /* ﾒ ･ (0xBF) */
  { VK_OEM_COMMA,  KANALOK, {',', '<', 0xff88, 0xff64} },  /* ﾈ ､ (0xBC) */
  { VK_OEM_PERIOD, KANALOK, {'.', '>', 0xff99, 0xff61} },  /* ﾙ ｡ (0xBE) */
  { VK_OEM_PLUS,   KANALOK, {';', '+', 0xff9a, 0xff9a} },  /* ﾚ  (0xBB) */
  { VK_TAB,        0, {'\t', '\t', '\t', '\t'} },
  { VK_ADD,        0, {'+', '+', '+', '+'} },
  { VK_DECIMAL,    0, {'.', '.', '.', '.'} },
  { VK_DIVIDE,     0, {'/', '/', '/', '/'} },
  { VK_MULTIPLY,   0, {'*', '*', '*', '*'} },
  { VK_SUBTRACT,   0, {'-', '-', '-', '-'} },
  { VK_SEPARATOR,  0, {',', ',', ',', ','} },
  { VK_OEM_NEC_EQUAL, 0, {'=', '=', '=', '='} },
  { 0, 0 }
};

/* Normal / Shift / Kana / Kana+Shift / Ctrl / Kana+Ctrl */
ROSDATA VK_TO_WCHARS6 key_to_chars_6mod[] = {
  { VK_BACK,    0, {0x08, 0x08, 0x08, 0x08, 0x7f, 0x7f} },
  { VK_CANCEL,  0, {0x03, 0x03, 0x03, 0x03, 0x03, 0x03} },
  { VK_ESCAPE,  0, {0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b} },
  { VK_OEM_3,   KANALOK, {'@',  '~',  0xff9e, 0xff9e, 0x00, 0x00} },   /* ﾞ        (0xC0) */
  { VK_OEM_4,   KANALOK, {'[',  '{',  0xff9f, 0xff62, 0x1b, 0x1b} },   /* ﾟ ｢      (0xDB) */
  { VK_OEM_5,   KANALOK, {'\\', '|',  0xff70, 0xff70, 0x1c, 0x1c} },   /* ｰ        (0xDC) */
  { VK_OEM_6,   KANALOK, {']',  '}',  0xff91, 0xff63, 0x1d, 0x1d} },   /* ﾑ ｣      (0xDD) */
  { VK_OEM_7,   KANALOK, {'^',  '`',  0xff8d, 0xff8d, 0x1e, 0x1e} },   /* ﾍ        (0xDE) */
  { VK_OEM_8,   KANALOK, {WCH_NONE, '_', 0xff9b, 0xff9b, 0x1f, 0x1f} },/* ﾛ        (0xDF) */
  { VK_RETURN,  0, {'\r', '\r', '\r', '\r', '\n', '\n'} },
  { VK_SPACE,   0, {' ',  ' ',  ' ',  ' ',  ' ',  ' '} },
  { 0, 0 }
};

/* Normal / Shift / Kana / Kana+Shift / Ctrl / Kana+Ctrl / Ctrl+Shift / Kana+Ctrl+Shift */
ROSDATA VK_TO_WCHARS8 key_to_chars_8mod[] = {
  { '2',          KANALOK, {'2', '"', 0xff8c, 0xff8c, WCH_NONE, WCH_NONE, 0x00, 0x00} },  /* ﾌ */
  { '6',          KANALOK, {'6', '&', 0xff75, 0xff6b, WCH_NONE, WCH_NONE, 0x1e, 0x1e} },  /* ｵ ｫ */
  { VK_OEM_MINUS, KANALOK, {'-', '=', 0xff8e, 0xff8e, WCH_NONE, WCH_NONE, 0x1f, 0x1f} },  /* ﾎ */
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 keypad_numbers[] = {
  { VK_NUMPAD0, 0, {'0', WCH_NONE, '0', WCH_NONE} },
  { VK_NUMPAD1, 0, {'1', WCH_NONE, '1', WCH_NONE} },
  { VK_NUMPAD2, 0, {'2', WCH_NONE, '2', WCH_NONE} },
  { VK_NUMPAD3, 0, {'3', WCH_NONE, '3', WCH_NONE} },
  { VK_NUMPAD4, 0, {'4', WCH_NONE, '4', WCH_NONE} },
  { VK_NUMPAD5, 0, {'5', WCH_NONE, '5', WCH_NONE} },
  { VK_NUMPAD6, 0, {'6', WCH_NONE, '6', WCH_NONE} },
  { VK_NUMPAD7, 0, {'7', WCH_NONE, '7', WCH_NONE} },
  { VK_NUMPAD8, 0, {'8', WCH_NONE, '8', WCH_NONE} },
  { VK_NUMPAD9, 0, {'9', WCH_NONE, '9', WCH_NONE} },
  { 0, 0 }
};

#define vk_master(n, x) { (PVK_TO_WCHARS1)x, n, sizeof(x[0]) }

ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(6, key_to_chars_6mod),
  vk_master(8, key_to_chars_8mod),
  vk_master(4, key_to_chars_4mod),
  vk_master(4, keypad_numbers),
  { 0, 0, 0 }
};

/* ------------------------------------------------------------------ */
/* Key names                                                          */
/* ------------------------------------------------------------------ */
ROSDATA VSC_LPWSTR key_names[] = {
  { 0x01, L"ESC" },
  { 0x0e, L"BS" },
  { 0x0f, L"TAB" },
  { 0x1c, L"RETURN" },
  { 0x1d, L"CTRL" },
  { 0x29, L"\u534A\u89D2/\u5168\u89D2" }, /* Hankaku/Zenkaku */
  { 0x2a, L"SHIFT" },
  { 0x36, L"SHIFT" },
  { 0x37, L"Num *" },
  { 0x38, L"GRPH" },
  { 0x39, L"SPACE" },
  { 0x3a, L"CAPS" },
  { 0x3b, L"f\xff65" L"1" },
  { 0x3c, L"f\xff65" L"2" },
  { 0x3d, L"f\xff65" L"3" },
  { 0x3e, L"f\xff65" L"4" },
  { 0x3f, L"f\xff65" L"5" },
  { 0x40, L"f\xff65" L"6" },
  { 0x41, L"f\xff65" L"7" },
  { 0x42, L"f\xff65" L"8" },
  { 0x43, L"f\xff65" L"9" },
  { 0x44, L"f\xff65" L"10" },
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
  { 0x53, L"Num ." },
  { 0x54, L"Sys Req" },
  { 0x57, L"f\xff65" L"11" },
  { 0x58, L"f\xff65" L"12" },
  { 0x59, L"Num =" },
  { 0x5c, L"Num ," },
  { 0x5d, L"f\xff65" L"13" },
  { 0x5e, L"f\xff65" L"14" },
  { 0x5f, L"f\xff65" L"15" },
  { 0x70, L"\xff76\xff85" },   /* Kana */
  { 0x79, L"XFER" },
  { 0x7b, L"NFER" },
  { 0, NULL },
};

ROSDATA VSC_LPWSTR extended_key_names[] = {
  { 0x1c, L"Num Enter" },
  { 0x35, L"Num /" },
  { 0x37, L"COPY" },
  { 0x45, L"Num Lock" },
  { 0x46, L"STOP" },
  { 0x47, L"CLR" },
  { 0x48, L"UP" },
  { 0x49, L"ROLL DOWN" },
  { 0x4b, L"LEFT" },
  { 0x4d, L"RIGHT" },
  { 0x4f, L"HELP" },
  { 0x50, L"DOWN" },
  { 0x51, L"ROLL UP" },
  { 0x52, L"INS" },
  { 0x53, L"DEL" },
  { 0x54, L"<00>" },
  { 0x56, L"HELP" },
  { 0x5b, L"Left Windows" },
  { 0x5c, L"Right Windows" },
  { 0x5d, L"Application" },
  { 0, NULL },
};

/* ------------------------------------------------------------------ */
/* Master table                                                       */
/* ------------------------------------------------------------------ */
ROSDATA KBDTABLES_FE keyboard_layout_table = {
  {
    &modifier_bits,           /* modifier assignments */
    vk_to_wchar_master_table, /* character from vk tables */
    NULL,                     /* diacritical marks -- none */

    /* Key names */
    (VSC_LPWSTR *)key_names,
    (VSC_LPWSTR *)extended_key_names,
    NULL, /* Dead key names */

    /* scan code to virtual key maps */
    scancode_to_vk,
    RTL_NUMBER_OF(scancode_to_vk),   /* 0x80 */
    extcode0_to_vk,
    extcode1_to_vk,

    0, /* fLocaleFlags */

    /* Ligatures -- none */
    0,
    0,
    NULL
  },
  7,      /* dwType    : Japanese keyboard */
  0x0d02  /* dwSubType : NEC PC-9800 (PC98-NX) */
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table.Base;
}

/* ------------------------------------------------------------------ */
/* NLS (Far-East) tables                                              */
/* ------------------------------------------------------------------ */

#define NLS_ENTRY(vk, p0, p1, p2, p3, p4, p5, p6, p7)                    \
  { (vk), KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,                    \
    { p0, p1, p2, p3, p4, p5, p6, p7 },                                  \
    { {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0} } }

#define NOP        { KBDNLS_NULL, 0 }
#define BASE       { KBDNLS_SEND_BASE_VK, 0 }
#define PARAM(vk)  { KBDNLS_SEND_PARAM_VK, (vk) }
#define FUNC(f)    { (f), 0 }

ROSDATA VK_F key_to_function[] = {
  /* Hankaku/Zenkaku */
  NLS_ENTRY(VK_DBE_SBCSCHAR,
            FUNC(KBDNLS_SBCSDBCS), FUNC(KBDNLS_SBCSDBCS), FUNC(KBDNLS_SBCSDBCS),
            FUNC(KBDNLS_SBCSDBCS), FUNC(KBDNLS_SBCSDBCS), FUNC(KBDNLS_SBCSDBCS),
            PARAM(VK_DBE_ENTERIMECONFIGMODE), PARAM(VK_DBE_ENTERIMECONFIGMODE)),

  /* NFER */
  NLS_ENTRY(VK_NONCONVERT,
            BASE, BASE, PARAM(VK_DBE_ALPHANUMERIC), FUNC(KBDNLS_SBCSDBCS),
            PARAM(VK_DBE_KATAKANA), PARAM(VK_DBE_ENTERWORDREGISTERMODE),
            PARAM(VK_DBE_NOCODEINPUT), PARAM(VK_DBE_FLUSHSTRING)),

  /* XFER */
  NLS_ENTRY(VK_CONVERT,
            BASE, BASE, PARAM(VK_KANJI), BASE,
            FUNC(KBDNLS_ROMAN), PARAM(VK_DBE_ENTERIMECONFIGMODE),
            PARAM(VK_DBE_CODEINPUT), PARAM(VK_DBE_HIRAGANA)),

  /* Ten-key = */
  NLS_ENTRY(VK_OEM_NEC_EQUAL,
            BASE, BASE, PARAM(VK_SCROLL), BASE, BASE, BASE, BASE, BASE),

  /* HOME / CLR */
  NLS_ENTRY(VK_HOME,
            FUNC(KBDNLS_HOME_OR_CLEAR), BASE, FUNC(KBDNLS_HOME_OR_CLEAR),
            FUNC(KBDNLS_HOME_OR_CLEAR), FUNC(KBDNLS_HOME_OR_CLEAR),
            FUNC(KBDNLS_HOME_OR_CLEAR), FUNC(KBDNLS_HOME_OR_CLEAR),
            FUNC(KBDNLS_HOME_OR_CLEAR)),

  /* END / HELP */
  NLS_ENTRY(VK_END,
            FUNC(KBDNLS_HELP_OR_END), FUNC(KBDNLS_HELP_OR_END),
            FUNC(KBDNLS_HELP_OR_END), FUNC(KBDNLS_HELP_OR_END),
            FUNC(KBDNLS_HELP_OR_END), FUNC(KBDNLS_HELP_OR_END),
            FUNC(KBDNLS_HELP_OR_END), FUNC(KBDNLS_HELP_OR_END)),

#define NLS_NUMPAD(vk) \
  NLS_ENTRY(vk, FUNC(KBDNLS_NUMPAD), FUNC(KBDNLS_NUMPAD), FUNC(KBDNLS_NUMPAD), \
                FUNC(KBDNLS_NUMPAD), FUNC(KBDNLS_NUMPAD), FUNC(KBDNLS_NUMPAD), \
                FUNC(KBDNLS_NUMPAD), FUNC(KBDNLS_NUMPAD))
  NLS_NUMPAD(VK_NUMPAD0),
  NLS_NUMPAD(VK_NUMPAD1),
  NLS_NUMPAD(VK_NUMPAD2),
  NLS_NUMPAD(VK_NUMPAD3),
  NLS_NUMPAD(VK_NUMPAD4),
  NLS_NUMPAD(VK_NUMPAD5),
  NLS_NUMPAD(VK_NUMPAD6),
  NLS_NUMPAD(VK_NUMPAD7),
  NLS_NUMPAD(VK_NUMPAD8),
  NLS_NUMPAD(VK_NUMPAD9),
};

/* Mouse keys */
ROSDATA USHORT mouse_vkeys[] = {
  VK_NUMPAD5, VK_NUMPAD9, VK_NUMPAD3, VK_NUMPAD1,
  VK_NUMPAD7, VK_NUMPAD4, VK_NUMPAD8, VK_NUMPAD6,
  VK_NUMPAD2, VK_NUMPAD0, VK_DECIMAL,  VK_MULTIPLY,
  VK_ADD,     VK_SUBTRACT,
  VK_DIVIDE | KBDEXT,   /* 0x16f */
  VK_HOME   | KBDEXT    /* 0x124 */
};

ROSDATA KBDNLSTABLES keyboard_nls_table = {
  0,                              /* OEMIdentifier */
  2,                              /* LayoutInformation */
  RTL_NUMBER_OF(key_to_function), /* 0x10 */
  key_to_function,
  RTL_NUMBER_OF(mouse_vkeys),     /* 0x10 */
  mouse_vkeys
};

PKBDNLSTABLES WINAPI KbdNlsLayerDescriptor(VOID) {
  return &keyboard_nls_table;
}
