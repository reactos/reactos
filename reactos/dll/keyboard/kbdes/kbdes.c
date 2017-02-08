/*
 * ReactOS Spanish Keyboard Layout
 * Copyright (C) 2003-2014 ReactOS
 * License: LGPL, see: LGPL.txt
 *
 * (c) 2003 - Created by HUMA2000 <huma2000@terra.es> from kbdus, kbdgr, kbdda and kbdfr.
 * (c) 2014 - Updated by Swyter   <swyterzone+ros@gmail.com>
 *
 * Thanks to: arty, for the kbtest utility and help.
 * Thanks to: carraca, from reactos.com forum for his fixes.
 * Thanks to: Elrond, for help.
 *
 * Thanks to: http://www.barcodeman.com/altek/mule/scandoc.php
 * and http://win.tue.nl/~aeb/linux/kbd/scancodes-1.html
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
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

#define VK_EMPTY  0xff                 /* The non-existent VK */

#define KNUMS     KBDNUMPAD|KBDSPECIAL /* Special + number pad */
#define KMEXT     KBDEXT|KBDMULTIVK    /* Multi + ext */

ROSDATA USHORT scancode_to_vk[] =
{
  /* 00 */  VK_EMPTY,
  /* 01 */  VK_ESCAPE,
  /* 02 */  L'1',
  /* 03 */  L'2',
  /* 04 */  L'3',
  /* 05 */  L'4',
  /* 06 */  L'5',
  /* 07 */  L'6',
  /* 08 */  L'7',
  /* 09 */  L'8',
  /* 0a */  L'9',
  /* 0b */  L'0',
  /* 0c */  VK_OEM_4,
  /* 0d */  VK_OEM_6,
  /* 0e */  VK_BACK,
  /* 0f */  VK_TAB,
  /* 10 */  L'Q',
  /* 11 */  L'W',
  /* 12 */  L'E',
  /* 13 */  L'R',
  /* 14 */  L'T',
  /* 15 */  L'Y',
  /* 16 */  L'U',
  /* 17 */  L'I',
  /* 18 */  L'O',
  /* 19 */  L'P',
  /* 1a */  VK_OEM_1,
  /* 1b */  VK_OEM_PLUS,
  /* 1c */  VK_RETURN,
  /* 1d */  VK_LCONTROL,
  /* 1e */  L'A',
  /* 1f */  L'S',
  /* 20 */  L'D',
  /* 21 */  L'F',
  /* 22 */  L'G',
  /* 23 */  L'H',
  /* 24 */  L'J',
  /* 25 */  L'K',
  /* 26 */  L'L',
  /* 27 */  VK_OEM_3,
  /* 28 */  VK_OEM_7,
  /* 29 */  VK_OEM_5,
  /* 2a */  VK_LSHIFT,
  /* 2b */  VK_OEM_2,
  /* 2c */  L'Z',
  /* 2d */  L'X',
  /* 2e */  L'C',
  /* 2f */  L'V',
  /* 30 */  L'B',
  /* 31 */  L'N',
  /* 32 */  L'M',
  /* 33 */  VK_OEM_COMMA,
  /* 34 */  VK_OEM_PERIOD,
  /* 35 */  VK_OEM_MINUS,
  /* 36 */  VK_RSHIFT,
  /* 37 */  VK_MULTIPLY,
  /* 38 */  VK_LMENU,
  /* 39 */  VK_SPACE,
  /* 3a */  VK_CAPITAL,
  /* 3b */  VK_F1,
  /* 3c */  VK_F2,
  /* 3d */  VK_F3,
  /* 3e */  VK_F4,
  /* 3f */  VK_F5,
  /* 40 */  VK_F6,
  /* 41 */  VK_F7,
  /* 42 */  VK_F8,
  /* 43 */  VK_F9,
  /* 44 */  VK_F10,
  /* 45 */  VK_NUMLOCK | KMEXT,
  /* 46 */  VK_SCROLL | KBDMULTIVK,
  /* 47 */  VK_HOME | KNUMS,
  /* 48 */  VK_UP | KNUMS,
  /* 49 */  VK_PRIOR | KNUMS,
  /* 4a */  VK_SUBTRACT,
  /* 4b */  VK_LEFT | KNUMS,
  /* 4c */  VK_CLEAR | KNUMS,
  /* 4d */  VK_RIGHT | KNUMS,
  /* 4e */  VK_ADD,
  /* 4f */  VK_END | KNUMS,
  /* 50 */  VK_DOWN | KNUMS,
  /* 51 */  VK_NEXT | KNUMS,
  /* 52 */  VK_INSERT | KNUMS,
  /* 53 */  VK_DELETE | KNUMS,
  /* 54 */  VK_SNAPSHOT,
  /* 55 */  VK_EMPTY,
  /* 56 */  VK_OEM_102,
  /* 57 */  VK_F11,
  /* 58 */  VK_F12,
  /* 59 */  VK_EMPTY,
  /* 5a */  VK_CLEAR,
  /* 5b */  VK_EMPTY,
  /* 5c */  VK_EMPTY,
  /* 5d */  VK_EMPTY,
  /* 5e */  VK_EMPTY, /* EREOF */
  /* 5f */  VK_EMPTY,
  /* 60 */  VK_EMPTY,
  /* 61 */  VK_EMPTY,
  /* 62 */  VK_EMPTY,
  /* 63 */  VK_EMPTY, /* ZOOM */
  /* 64 */  VK_HELP,
  /* 65 */  VK_F13,
  /* 66 */  VK_F14,
  /* 67 */  VK_F15,
  /* 68 */  VK_F16,
  /* 69 */  VK_F17,
  /* 6a */  VK_F18,
  /* 6b */  VK_F19,
  /* 6c */  VK_F20,
  /* 6d */  VK_F21,
  /* 6e */  VK_F22,
  /* 6f */  VK_F23,
  /* 70 */  VK_EMPTY,
  /* 71 */  VK_EMPTY,
  /* 72 */  VK_EMPTY,
  /* 73 */  VK_EMPTY,
  /* 74 */  VK_EMPTY,
  /* 75 */  VK_EMPTY,
  /* 76 */  VK_EMPTY,
  /* 77 */  VK_F24,
  /* 78 */  VK_EMPTY,
  /* 79 */  VK_EMPTY,
  /* 7a */  VK_EMPTY,
  /* 7b */  VK_EMPTY,
  /* 7c */  VK_EMPTY,
  /* 7d */  VK_EMPTY,
  /* 7e */  VK_EMPTY,
  /* 7f */  VK_EMPTY,
  /* 80 */  VK_EMPTY,
  /* 00 */  0
};

ROSDATA VSC_VK extcode0_to_vk[] =
{
  { 0x10, KBDEXT | VK_MEDIA_PREV_TRACK }, // Pista anterior, no puedo probarlo hasta que no se implemente el sonido
  { 0x19, KBDEXT | VK_MEDIA_NEXT_TRACK }, // Pista siguiente,
  { 0x1D, KBDEXT | VK_RCONTROL }, // Tecla control
  { 0x20, KBDEXT | VK_VOLUME_MUTE }, // Silenciar volumen
  { 0x21, KBDEXT | VK_LAUNCH_APP2 }, // Tecla calculadora
  { 0x22, KBDEXT | VK_MEDIA_PLAY_PAUSE }, // Reproducir/Pausa
  { 0x24, KBDEXT | VK_MEDIA_STOP }, // Stop
  { 0x2E, KBDEXT | VK_VOLUME_DOWN }, // Bajar volumen
  { 0x30, KBDEXT | VK_VOLUME_UP }, // Subir volumen
  { 0x32, KBDEXT | VK_BROWSER_HOME }, // Pagina de inicio del navegador de Internet o abrirlo si no esta activo
  { 0x35, KBDEXT | VK_DIVIDE }, // Tecla
  { 0x37, KBDEXT | VK_SNAPSHOT }, // La tecla de imprimir pantalla
  { 0x38, KBDEXT | VK_RMENU }, // Tecla Alt
  { 0x47, KBDEXT | VK_HOME }, // Tecla inicio
  { 0x48, KBDEXT | VK_UP }, // Cursor arriba
  { 0x49, KBDEXT | VK_PRIOR }, // Tecla Repág
  { 0x4B, KBDEXT | VK_LEFT }, // Cursor izquierda
  { 0x4D, KBDEXT | VK_RIGHT }, // Cursor derecha
  { 0x4F, KBDEXT | VK_END }, // Tecla Fin
  { 0x50, KBDEXT | VK_DOWN }, // Cursor abajo
  { 0x51, KBDEXT | VK_NEXT }, // Tecla Avpág
  { 0x52, KBDEXT | VK_INSERT }, // Tecla Ins
  { 0x53, KBDEXT | VK_DELETE }, // Tecla Supr
  { 0x5B, KBDEXT | VK_LWIN }, // Tecla Windows izquierda
  { 0x5C, KBDEXT | VK_RWIN }, // Tecla Windows derecha
  { 0x5D, KBDEXT | VK_APPS }, // Tecla menú aplicacion derecha
  { 0x5F, KBDEXT | VK_SLEEP }, // Tecla Suspensión
  { 0x65, KBDEXT | VK_BROWSER_SEARCH }, // Pagina de búsqueda en el navegador de Internet
  { 0x66, KBDEXT | VK_BROWSER_FAVORITES }, // Favoritos, tengo que esperar a que el tcp/ip
  { 0x67, KBDEXT | VK_BROWSER_REFRESH }, // Refrescar el navegador de Internet
  { 0x68, KBDEXT | VK_BROWSER_STOP }, // Stop en el navegador de Internet
  { 0x69, KBDEXT | VK_BROWSER_FORWARD }, // Adelante en el navegador de Internet
  { 0x6A, KBDEXT | VK_BROWSER_BACK }, // Atrás en el navegador de Internet
  { 0x6B, KBDEXT | VK_LAUNCH_APP1 }, // Tecla Mi PC
  { 0x6C, KBDEXT | VK_LAUNCH_MAIL }, // Abrir programa de e-mail
  { 0x6D, KBDEXT | VK_LAUNCH_MEDIA_SELECT }, // Abrir reproductor multimedia
  { 0x1C, KBDEXT | VK_RETURN }, // La tecla de intro
  { 0x46, KBDEXT | VK_CANCEL }, // Tecla escape
  { 0, 0 },
};

ROSDATA VSC_VK extcode1_to_vk[] =
{
  { 0, 0 },
};

ROSDATA VK_TO_BIT modifier_keys[] =
{
  { VK_SHIFT,   KBDSHIFT },
  { VK_CONTROL, KBDCTRL },
  { VK_MENU,    KBDALT },
  { 0,  0 }
};

ROSDATA MODIFIERS modifier_bits =
{
  modifier_keys,
  6,
  {
    0,   /* NINGUNO    */
    1,   /* MAYÚS      */
    2,   /* CTRL       */
    2|1, /* CTRL+MAYÚS */
    SHFT_INVALID,
    SHFT_INVALID,
    3    /* ALT        */
  }
};

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] =
{
  /* Normal, Mayús */
  /* The numbers -- Del 1 al 6 tienen tres estados */
  { L'7',           0,  {L'7',   L'/'} },
  { L'8',           0,  {L'8',   L'('} },
  { L'9',           0,  {L'9',   L')'} },
  { L'0',           0,  {L'0',   L'='} },
  { VK_OEM_3,  CAPLOK,  {L'ñ',   L'Ñ'} }, /* ñÑ */

  /* Specials */
  /* Ctrl-_ generates ES */
  { VK_OEM_6,       0,  {L'¡',   L'¿'} }, /* ¡¿ */
  { VK_OEM_4,       0,  {0x27,   L'?'} }, /* '? */
  { VK_OEM_COMMA,   0,  {L',',   L';'} },
  { VK_OEM_PERIOD,  0,  {L'.',   L':'} },
  { VK_OEM_MINUS,   0,  {L'-',   L'_'} },
  { VK_OEM_102,     0,  {L'<',   L'>'} },

  /* Keys that do not have shift states */
  { VK_TAB,         0,  {L'\t', L'\t'} },
  { VK_ADD,         0,  {L'+',   L'+'} },
  { VK_SUBTRACT,    0,  {L'-',   L'-'} },
  { VK_MULTIPLY,    0,  {L'*',   L'*'} },
  { VK_DIVIDE,      0,  {L'/',   L'/'} },
  { VK_ESCAPE,      0,  {0x1b,   0x1b} },
  { VK_SPACE,       0,  {L' ',   L' '} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] =
{
  /* Normal, Mayús, Ctrl */
  /* The alphabet */
  { L'A',      CAPLOK,  {L'a', L'A', 0x01} },
  { L'B',      CAPLOK,  {L'b', L'B', 0x02} },
  { L'C',      CAPLOK,  {L'c', L'C', 0x03} },
  { L'D',      CAPLOK,  {L'd', L'D', 0x04} },
  { L'F',      CAPLOK,  {L'f', L'F', 0x06} },
  { L'G',      CAPLOK,  {L'g', L'G', 0x07} },
  { L'H',      CAPLOK,  {L'h', L'H', 0x08} },
  { L'I',      CAPLOK,  {L'i', L'I', 0x09} },
  { L'J',      CAPLOK,  {L'j', L'J', 0x0a} },
  { L'K',      CAPLOK,  {L'k', L'K', 0x0b} },
  { L'L',      CAPLOK,  {L'l', L'L', 0x0c} },
  { L'M',      CAPLOK,  {L'm', L'M', 0x0d} },
  { L'N',      CAPLOK,  {L'n', L'N', 0x0e} },
  { L'O',      CAPLOK,  {L'o', L'O', 0x0f} },
  { L'P',      CAPLOK,  {L'p', L'P', 0x10} },
  { L'Q',      CAPLOK,  {L'q', L'Q', 0x11} },
  { L'R',      CAPLOK,  {L'r', L'R', 0x12} },
  { L'S',      CAPLOK,  {L's', L'S', 0x13} },
  { L'T',      CAPLOK,  {L't', L'T', 0x14} },
  { L'U',      CAPLOK,  {L'u', L'U', 0x15} },
  { L'V',      CAPLOK,  {L'v', L'V', 0x16} },
  { L'W',      CAPLOK,  {L'w', L'W', 0x17} },
  { L'X',      CAPLOK,  {L'x', L'X', 0x18} },
  { L'Y',      CAPLOK,  {L'y', L'Y', 0x19} },
  { L'Z',      CAPLOK,  {L'z', L'Z', 0x1a} },

  /* Legacy (telnet-style) ascii escapes */
  { VK_RETURN,      0,  {L'\r',L'\r',L'\n'} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] =
{
  /* Normal, Mayús, Ctrl, Alt+Gr */
  { L'1',           0,  {L'1',      L'!',     WCH_NONE, L'|'}     }, /* 1!| */
  { L'2',           0,  {L'2',      L'"',     WCH_NONE, L'@'}     }, /* 2"@ */
  { L'3',           0,  {L'3',      L'·',     WCH_NONE, L'#'}     }, /* 3·# */
  { L'4',           0,  {L'4',      L'$',     WCH_NONE, WCH_DEAD} }, /* 4$~ */
  { VK_EMPTY,       0,  {WCH_NONE,  WCH_NONE, WCH_NONE, L'~'}     }, /* 4$~ */
  { L'5',           0,  {L'5',      L'%',     WCH_NONE, L'€'}     }, /* 5%€ */
  { L'6',           0,  {L'6',      L'&',     WCH_NONE, L'¬'}     }, /* 6&¬ */
  { L'E',      CAPLOK,  {L'e',      L'E',     0x05,     L'€'}     }, /* eE€ */
  { VK_OEM_PLUS,    0,  {L'+',      L'*',     WCH_NONE, L']'}     }, /* +*] */
  { VK_OEM_2,       0,  {L'ç',      L'Ç',     WCH_NONE, L'}'}     }, /* çÇ} */

  { VK_OEM_7,       0,  {WCH_DEAD,  WCH_DEAD, WCH_NONE, L'{'}     }, /* ´¨{ */
  { VK_EMPTY,       0,  {L'´',      L'¨',     WCH_NONE, WCH_NONE} }, /* ´¨{ */

  { VK_OEM_1,       0,  {WCH_DEAD,  WCH_DEAD, WCH_NONE, L'['}     }, /* `^[ */
  { VK_EMPTY,       0,  {L'`',      L'^',     WCH_NONE, WCH_NONE} }, /* `^[ */

  { VK_OEM_5,       0,  {L'º',      L'ª',     WCH_NONE, 0x5c}     }, /* ºª\ */
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS1 keypad_numbers[] =
{
  { VK_NUMPAD0,     0,  {L'0'} },
  { VK_NUMPAD1,     0,  {L'1'} },
  { VK_NUMPAD2,     0,  {L'2'} },
  { VK_NUMPAD3,     0,  {L'3'} },
  { VK_NUMPAD4,     0,  {L'4'} },
  { VK_NUMPAD5,     0,  {L'5'} },
  { VK_NUMPAD6,     0,  {L'6'} },
  { VK_NUMPAD7,     0,  {L'7'} },
  { VK_NUMPAD8,     0,  {L'8'} },
  { VK_NUMPAD9,     0,  {L'9'} },
  { VK_DECIMAL,     0,  {L'.'} },
  { VK_BACK,        0,  {L'\010'} },
  { 0, 0 }
};

#define vk_master(n,x) { (PVK_TO_WCHARS1)x, n, sizeof(x[0]) }

ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] =
{
  vk_master(1, keypad_numbers),
  vk_master(2, key_to_chars_2mod),
  vk_master(3, key_to_chars_3mod),
  vk_master(4, key_to_chars_4mod),
  { 0, 0, 0 }
};

#define DEADTRANS(ch, accent, comp, flags) MAKELONG(ch, accent), comp, flags //no funciona
ROSDATA DEADKEY deadkey[] =
{
  /* ´ */
  { DEADTRANS(L'a', L'´', 0xE1, 0x00) }, // letra a con ´ | á
  { DEADTRANS(L'A', L'´', 0xC1, 0x00) }, // letra A con ´ | Á
  { DEADTRANS(L'e', L'´', 0xE9, 0x00) }, // letra e con ´ | é
  { DEADTRANS(L'E', L'´', 0xC9, 0x00) }, // letra E con ´ | É
  { DEADTRANS(L'i', L'´', 0xED, 0x00) }, // letra i con ´ | í
  { DEADTRANS(L'I', L'´', 0xCD, 0x00) }, // letra I con ´ | Í
  { DEADTRANS(L'o', L'´', 0xF3, 0x00) }, // letra o con ´ | ó
  { DEADTRANS(L'O', L'´', 0xD3, 0x00) }, // letra O con ´ | Ó
  { DEADTRANS(L'u', L'´', 0xFA, 0x00) }, // letra u con ´ | ú
  { DEADTRANS(L'U', L'´', 0xDA, 0x00) }, // letra U con ´ | Ú

  /* ` */
  { DEADTRANS(L'a', L'`', 0xE0, 0x00) }, // letra a con ` | à
  { DEADTRANS(L'A', L'`', 0xC0, 0x00) }, // letra A con ` | À
  { DEADTRANS(L'e', L'`', 0xE8, 0x00) }, // letra e con ` | è
  { DEADTRANS(L'E', L'`', 0xC8, 0x00) }, // letra E con ` | È
  { DEADTRANS(L'i', L'`', 0xEC, 0x00) }, // letra i con ` | ì
  { DEADTRANS(L'I', L'`', 0xCC, 0x00) }, // letra I con ` | Ì
  { DEADTRANS(L'o', L'`', 0xF2, 0x00) }, // letra o con ` | ò
  { DEADTRANS(L'O', L'`', 0xD2, 0x00) }, // letra O con ` | Ò
  { DEADTRANS(L'u', L'`', 0xF9, 0x00) }, // letra u con ` | ù
  { DEADTRANS(L'U', L'`', 0xD9, 0x00) }, // letra U con ` | Ù

  /* ^ */
  { DEADTRANS(L'a', L'^', 0xE2, 0x00) }, // letra a con ^ | â
  { DEADTRANS(L'A', L'^', 0xC2, 0x00) }, // letra A con ^ | Â
  { DEADTRANS(L'e', L'^', 0xEA, 0x00) }, // letra e con ^ | ê
  { DEADTRANS(L'E', L'^', 0xCA, 0x00) }, // letra E con ^ | Ê
  { DEADTRANS(L'i', L'^', 0xEE, 0x00) }, // letra i con ^ | î
  { DEADTRANS(L'I', L'^', 0xCE, 0x00) }, // letra I con ^ | Î
  { DEADTRANS(L'o', L'^', 0xF4, 0x00) }, // letra o con ^ | ô
  { DEADTRANS(L'O', L'^', 0xD4, 0x00) }, // letra O con ^ | Ô
  { DEADTRANS(L'u', L'^', 0xFB, 0x00) }, // letra u con ^ | û
  { DEADTRANS(L'U', L'^', 0xDB, 0x00) }, // letra U con ^ | Û

  /* ¨ */
  { DEADTRANS(L'a', L'¨', 0xE4, 0x00) }, // letra a con ¨ | ä
  { DEADTRANS(L'A', L'¨', 0xC4, 0x00) }, // letra A con ¨ | Ä
  { DEADTRANS(L'e', L'¨', 0xEB, 0x00) }, // letra e con ¨ | ë
  { DEADTRANS(L'E', L'¨', 0xCB, 0x00) }, // letra E con ¨ | Ë
  { DEADTRANS(L'i', L'¨', 0xEF, 0x00) }, // letra i con ¨ | ï
  { DEADTRANS(L'I', L'¨', 0xCF, 0x00) }, // letra I con ¨ | Ï
  { DEADTRANS(L'o', L'¨', 0xF6, 0x00) }, // letra o con ¨ | ö
  { DEADTRANS(L'O', L'¨', 0xD6, 0x00) }, // letra O con ¨ | Ö
  { DEADTRANS(L'u', L'¨', 0xFC, 0x00) }, // letra u con ¨ | ü
  { DEADTRANS(L'U', L'¨', 0xDC, 0x00) }, // letra U con ¨ | Ü

  /* ~ */
  { DEADTRANS(L'a', L'~', 0xe3, 0x00) }, // letra a con ~ | ã
  { DEADTRANS(L'A', L'~', 0xc3, 0x00) }, // letra A con ~ | Ã
  { DEADTRANS(L'n', L'~', 0xf1, 0x00) }, // letra n con ~ | ñ
  { DEADTRANS(L'N', L'~', 0xd1, 0x00) }, // letra N con ~ | Ñ
  { DEADTRANS(L'o', L'~', 0xf5, 0x00) }, // letra o con ~ | õ
  { DEADTRANS(L'O', L'~', 0xd5, 0x00) }, // letra O con ~ | Õ

  { 0, 0, 0 }
};

ROSDATA VSC_LPWSTR key_names[] =
{
  { 0x00, L"" },
  { 0x01, L"Esc" },
  { 0x0e, L"Retroceso" },
  { 0x0f, L"Tab" },
  { 0x1c, L"Intro" },
  { 0x1d, L"Ctrl" },
  { 0x2a, L"Mayús" },
  { 0x36, L"Mayús der." },
  { 0x37, L"* num" },
  { 0x38, L"Alt" },
  { 0x39, L"Espacio" },
  { 0x3a, L"Bloq mayús" },
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
  { 0x45, L"Pausa" },
  { 0x46, L"Bloqueo despl." },
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
  { 0x53, L"Supr numér." },
  { 0x54, L"Pet sist" },
  { 0x57, L"F11" },
  { 0x58, L"F12" },
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
  { 0, NULL },
};

ROSDATA VSC_LPWSTR extended_key_names[] =
{
  { 0x1c, L"Intro num" },
  { 0x1d, L"Ctrl der." },
  { 0x35, L"/ num." },
  { 0x37, L"Impr pant." },
  { 0x38, L"Alt der." },
  { 0x45, L"Bloq num" },
  { 0x46, L"Int" },
  { 0x47, L"Inicio" },
  { 0x48, L"Arriba" },
  { 0x49, L"Repág" },
  { 0x4b, L"Izquierda" },
  { 0x4c, L"Centrar" },
  { 0x4d, L"Derecha" },
  { 0x4f, L"Fin" },
  { 0x50, L"Abajo" },
  { 0x51, L"Avpág" },
  { 0x52, L"Ins" },
  { 0x53, L"Supr" },
  { 0x54, L"<ReactOS>" },
  { 0x55, L"Ayuda" },
  { 0x5b, L"Windows izq." },
  { 0x5c, L"Windows der." },
  { 0, NULL },
};

ROSDATA DEADKEY_LPWSTR dead_key_names[] =
{
  L"\x00b4" L"Agudo",
  L"\x0060" L"Grave",
  L"\x005e" L"Circunflejo",
  L"\x00A8" L"Diéresis",
  NULL
};

/* Finally, the master table */
ROSDATA KBDTABLES keyboard_layout_table =
{
  /* modifier assignments */
  &modifier_bits,

  /* character from vk tables */
  vk_to_wchar_master_table,

  /* diacritical marks -- currently implemented by wine code, spanish has several */
  deadkey,

  /* Key names */
  (VSC_LPWSTR *)key_names,
  (VSC_LPWSTR *)extended_key_names,

  /* Dead key names */
  dead_key_names,

  /* scan code to virtual key maps */
  scancode_to_vk,
  sizeof(scancode_to_vk) / sizeof(scancode_to_vk[0]),
  extcode0_to_vk,
  extcode1_to_vk,

  /* version 1.0 */
  MAKELONG(KLLF_ALTGR, 1),

  /* ligatures -- Spanish does not have any */
  0,
  0,
  NULL
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID)
{
  return &keyboard_layout_table;
}
