/*
 * ReactOS POASCII Keyboard layout
 * Copyright (C) 2006 ReactOS
 * License: LGPL, see: LGPL.txt
 * Based on kbdes, skbdda and skbdfr
 *
 * Made by Pedro Maia pedrom.maia@gmail.com
 *
 *
 * http://keytouch.sourceforge.net/howto_keyboard/node4.html (Very Good)
 * http://www.microsoft.com/globaldev/tools/msklc.mspx (Keyboard layout file from MS)
 *
 *TODO
 *	Correct DEADKEYS
 *	Correct DIVIDE and PrtScr
 */

#include <windows.h>
#include <internal/kbd.h>

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


#define VK_EMPTY 0xff   /* The non-existent VK */
#define KSHIFT   0x001  /* Shift modifier */
#define KCTRL    0x002  /* Ctrl modifier */
#define KALT     0x004  /* Alt modifier */
#define KEXT     0x100  /* Extended key code */
#define KMULTI   0x200  /* Multi-key */
#define KSPEC    0x400  /* Special key */
#define KNUMP    0x800  /* Number-pad */
#define KNUMS    0xc00  /* Special + number pad */
#define KMEXT    0x300  /* Multi + ext */

#define SHFT_INVALID 0x0F

ROSDATA USHORT scancode_to_vk[] = {
/* 00 */  VK_EMPTY,
/* 01 */  VK_ESCAPE,
/* 02 */  '1',
/* 03 */  '2',
/* 04 */  '3',
/* 05 */  '4',
/* 06 */  '5',
/* 07 */  '6',
/* 08 */  '7',
/* 09 */  '8',
/* 0a */  '9',
/* 0b */  '0',
/* 0c */  VK_OEM_4,
/* 0d */  VK_OEM_6,
/* 0e */  VK_BACK,
/* 0f */  VK_TAB,
/* 10 */  'Q',
/* 11 */  'W',
/* 12 */  'E',
/* 13 */  'R',
/* 14 */  'T',
/* 15 */  'Y',
/* 16 */  'U',
/* 17 */  'I',
/* 18 */  'O',
/* 19 */  'P',
/* 1a */  VK_OEM_PLUS,
/* 1b */  VK_OEM_1,
/* 1c */  VK_RETURN,
/* 1d */  VK_LCONTROL,
/* 1e */  'A',
/* 1f */  'S',
/* 20 */  'D',
/* 21 */  'F',
/* 22 */  'G',
/* 23 */  'H',
/* 24 */  'J',
/* 25 */  'K',
/* 26 */  'L',
/* 27 */  VK_OEM_3,
/* 28 */  VK_OEM_7,
/* 29 */  VK_OEM_5,
/* 2a */  VK_LSHIFT,
/* 2b */  VK_OEM_2,
/* 2c */  'Z',
/* 2d */  'X',
/* 2e */  'C',
/* 2f */  'V',
/* 30 */  'B',
/* 31 */  'N',
/* 32 */  'M',
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
/* 46 */  VK_SCROLL | KMULTI,
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
/* 5e */  VK_EMPTY,  /* EREOF */
/* 5f */  VK_EMPTY,
/* 60 */  VK_EMPTY,
/* 61 */  VK_EMPTY,
/* 62 */  VK_EMPTY,
/* 63 */  VK_EMPTY,  /* ZOOM */
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

ROSDATA VSC_VK extcode0_to_vk[] = {
  { 0x10, VK_MEDIA_PREV_TRACK    | KEXT },	// Pista Anterior
  { 0x19, VK_MEDIA_NEXT_TRACK    | KEXT },	// Proxima Pista
  { 0x1D, VK_RCONTROL            | KEXT },	// Tecla ctrl
  { 0x20, VK_VOLUME_MUTE         | KEXT },	// Mute volume
  { 0x21, VK_LAUNCH_APP2         | KEXT },	// Tecla calculadora
  { 0x22, VK_MEDIA_PLAY_PAUSE    | KEXT },	// Play/pause
  { 0x24, VK_MEDIA_STOP          | KEXT },	// Stop
  { 0x2E, VK_VOLUME_DOWN         | KEXT },	// Baixar volume
  { 0x30, VK_VOLUME_UP           | KEXT },	// Subir volume
  { 0x32, VK_BROWSER_HOME        | KEXT },	// Pagina predefinida do navegador de internet, ou abri-lo se não estiver activo
  { 0x35, VK_DIVIDE              | KEXT },	// Tecla /
  { 0x37, VK_SNAPSHOT            | KEXT },	// Tecla de Print Screen
  { 0x38, VK_RMENU               | KEXT },	// Tecla Alt
  { 0x47, VK_HOME                | KEXT },	// Tecla Home
  { 0x48, VK_UP                  | KEXT },	// Cursor Cima
  { 0x49, VK_PRIOR               | KEXT },	// Tecla Re pag
  { 0x4b, VK_LEFT                | KEXT },	// Cursor esquerda
  { 0x4d, VK_RIGHT               | KEXT },	// Cursor direita
  { 0x4f, VK_END                 | KEXT },	// Tecla End
  { 0x50, VK_DOWN                | KEXT },	// Cursor Down
  { 0x51, VK_NEXT                | KEXT },	// Tecla Av pag
  { 0x52, VK_INSERT              | KEXT },	// Tecla insert
  { 0x53, VK_DELETE              | KEXT },	// Tecla delete
  { 0x5b, VK_LWIN                | KEXT },	// Tecla windows esquerda
  { 0x5c, VK_RWIN                | KEXT },	// Tecla windows direita
  { 0x5d, VK_APPS                | KEXT },	// Tecla menu aplicacao direita*/
  { 0x5f, VK_SLEEP               | KEXT },	// Tecla Sleep
  { 0x65, VK_BROWSER_SEARCH      | KEXT },	// Pagina de pesquisa do navegador de internet
  { 0x66, VK_BROWSER_FAVORITES   | KEXT },	// Favoritos, not yet implemented
  { 0x67, VK_BROWSER_REFRESH     | KEXT },	// Actualizar pagina do navegador de internet
  { 0x68, VK_BROWSER_STOP        | KEXT },	// Parar navegação na internet internet
  { 0x69, VK_BROWSER_FORWARD     | KEXT },	// Frente no historico de paginas no navegador de internet
  { 0x6a, VK_BROWSER_BACK        | KEXT },	// Atras no historico de paginas no navegador de internet (Backspace)
  { 0x6b, VK_LAUNCH_APP1         | KEXT },	// Tecla Meu Computador
  { 0x6c, VK_LAUNCH_MAIL         | KEXT },	// Abrir programa de e-mail
  { 0x6d, VK_LAUNCH_MEDIA_SELECT | KEXT },	// Abrir reproductor multimedia
  { 0x1c, VK_RETURN              | KEXT },	// Tecla de Enter
  { 0x46, VK_CANCEL              | KEXT },	// Tecla Escape
  { 0, 0 },
};

ROSDATA VSC_VK extcode1_to_vk[] = {
   { 0, 0 },
};






#define	TIDLE_CIRC		VK_OEM_2
#define	ACUTE_GRAVE		VK_OEM_1
#define	ORDERN_SUPERSCRIPT	VK_OEM_7
#define	CCEDIL			VK_OEM_3
#define	QUOTE			VK_OEM_4
#define	BACKSLASH_BAR		VK_OEM_5
#define	CLASSIC_QUOTES		VK_OEM_6
#define	MATH_RELATE		VK_OEM_102



#define	ACUTE_CHAR	0xB4
#define	GRAVE_CHAR	0x60
#define	CIRC_CHAR	0x5E
#define	TIDLE_CHAR	0x7E
#define	TREMA_CHAR	0xA8


/* Modifiers */

ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT,   KSHIFT },
  { VK_CONTROL, KCTRL },
  { VK_MENU,    KALT },
  { 0,  0 }
};

ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  6,
  {     0,  1<<0, 1<<1, 1<<2, SHFT_INVALID, SHFT_INVALID,             3  }
 /*  NONE, SHIFT, CTRL,  ALT,         MENU, SHIFT + MENU, SHIFT+CONTROL  */
};






#define NOCAPS 0
#define CAPS   KSHIFT /* Caps -> shift */

ROSDATA VK_TO_WCHARS1 keypad_numbers[] = {
  { VK_NUMPAD0, 0, {'0'} },
  { VK_NUMPAD1, 0, {'1'} },
  { VK_NUMPAD2, 0, {'2'} },
  { VK_NUMPAD3, 0, {'3'} },
  { VK_NUMPAD4, 0, {'4'} },
  { VK_NUMPAD5, 0, {'5'} },
  { VK_NUMPAD6, 0, {'6'} },
  { VK_NUMPAD7, 0, {'7'} },
  { VK_NUMPAD8, 0, {'8'} },
  { VK_NUMPAD9, 0, {'9'} },
  { VK_DECIMAL, 0, {'.'} },
  { 0,0 }
};

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  /* Normal vs Shifted */

   /* The alphabet */
  { 'A',    CAPS,   {'a', 'A'} },
  { 'B',    CAPS,   {'b', 'B'} },
  { 'C',    CAPS,   {'c', 'C'} },
  { 'D',    CAPS,   {'d', 'D'} },
  { 'F',    CAPS,   {'f', 'F'} },
  { 'G',    CAPS,   {'g', 'G'} },
  { 'H',    CAPS,   {'h', 'H'} },
  { 'I',    CAPS,   {'i', 'I'} },
  { 'J',    CAPS,   {'j', 'J'} },
  { 'K',    CAPS,   {'k', 'K'} },
  { 'L',    CAPS,   {'l', 'L'} },
  { 'M',    CAPS,   {'m', 'M'} },
  { 'N',    CAPS,   {'n', 'N'} },
  { 'O',    CAPS,   {'o', 'O'} },
  { 'P',    CAPS,   {'p', 'P'} },
  { 'Q',    CAPS,   {'q', 'Q'} },
  { 'R',    CAPS,   {'r', 'R'} },
  { 'S',    CAPS,   {'s', 'S'} },
  { 'T',    CAPS,   {'t', 'T'} },
  { 'U',    CAPS,   {'u', 'U'} },
  { 'V',    CAPS,   {'v', 'V'} },
  { 'W',    CAPS,   {'w', 'W'} },
  { 'X',    CAPS,   {'x', 'X'} },
  { 'Y',    CAPS,   {'y', 'Y'} },
  { 'Z',    CAPS,   {'z', 'Z'} },

   /* The numbers */
  //De 2 ate 4 tem tres estados
  { '1',  NOCAPS,   {'1', '!'} },
  { '5',  NOCAPS,   {'5', '%'} },
  { '6',  NOCAPS,   {'6', '&'} },
  //De 7 ate 0 tem tres estados

  /* Specials */
  /* Shift-_ generates PT */
  { TIDLE_CIRC,		NOCAPS, {   WCH_DEAD,   WCH_DEAD} },
  {   VK_EMPTY,		NOCAPS, { TIDLE_CHAR,  CIRC_CHAR} },

  { CCEDIL,		  CAPS, {       0xe7,       0xc7} }, // ç
  { QUOTE,		NOCAPS, {       0xb4,        '?'} }, // ' ?
  { BACKSLASH_BAR,	NOCAPS, {       0x5c,       0x7c} }, // \ |
  { CLASSIC_QUOTES,	NOCAPS, {       0xab,       0xbb} }, // « »

  { ACUTE_GRAVE,  	NOCAPS, {   WCH_DEAD,   WCH_DEAD} }, // ` '
  {    VK_EMPTY,  	NOCAPS, { ACUTE_CHAR, GRAVE_CHAR} },

  { ORDERN_SUPERSCRIPT,	NOCAPS, {       0xBA,       0xAA} }, // º ª
  { MATH_RELATE,	NOCAPS, {        '<',        '>'} },
  { VK_OEM_COMMA,	NOCAPS, {        ',',        ';'} },
  { VK_OEM_PERIOD,	NOCAPS, {        '.',        ':'} },
  { VK_OEM_MINUS,	NOCAPS, {        '-',        '_'} },

  /* Keys that do not have shift states */
  { VK_TAB,		NOCAPS, {        '\t',      '\t'} },
  { VK_ADD,		NOCAPS, {         '+',       '+'} },
  { VK_SUBTRACT,	NOCAPS, {         '-',       '-'} },
  { VK_MULTIPLY,	NOCAPS, {         '*',       '*'} },
  { VK_DIVIDE,		NOCAPS, {         '/',       '/'} },
  { VK_ESCAPE,		NOCAPS, {      '\x1b',    '\x1b'} },
  { VK_SPACE,		NOCAPS, {         ' ',       ' '} },

  { 0, 0 }
};


ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  /* Normal, Shifted, Ctrl */

  /* Legacy (telnet-style) ascii escapes */
  { VK_RETURN,    NOCAPS, {'\r',     '\r',     '\n'} },
  { VK_BACK,      NOCAPS, {'\b',     '\b',     0x7f} },
  { 0,0 }
};


ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
                        /* Normal, Shift,  Control,   Alt+Gr */

  {         '2', NOCAPS, {    '2',   '"', WCH_NONE,        '@' }  }, // 2 " @
  {         '3', NOCAPS, {    '3',   '#', WCH_NONE,       0xa3 }  }, // 3 #
  {         '4', NOCAPS, {    '4',   '$', WCH_NONE,       0xa7 }  }, // 4 $
  {         '7', NOCAPS, {    '7',   '/', WCH_NONE,       0x7b }  }, // 7 &
  {         '8', NOCAPS, {    '8',   '(', WCH_NONE,       0x5b }  }, // 8 (
  {         '9', NOCAPS, {    '9',   ')', WCH_NONE,       0x5d }  }, // 9 )
  {         '0', NOCAPS, {    '0',   '=', WCH_NONE,       0x7d }  }, // 0 =
  {         'E',   CAPS, {    'e',   'E', WCH_NONE,     0x20ac }  }, // e E

  { VK_OEM_PLUS, NOCAPS, {    '+',   '*', WCH_NONE,   WCH_DEAD }  }, // + * "
  {    VK_EMPTY, NOCAPS, {    '+',   '*', WCH_NONE, TREMA_CHAR }  },
  { 0, 0 }
};








#define vk_master(n,x) { (PVK_TO_WCHARS1)x, n, sizeof(x[0]) }

ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(1,keypad_numbers),
  vk_master(2,key_to_chars_2mod),
  vk_master(3,key_to_chars_3mod),
  vk_master(4,key_to_chars_4mod),
  { 0,0,0 }
};











#define DEADTRANS(ch, accent, comp, flags) MAKELONG(ch, accent), comp, flags

ROSDATA DEADKEY  deadkey[] =
{
            //*´* DEADKEY	00B4
            { DEADTRANS(L'a', 0xb4, 0xe1, 0x00) }, // letra a com ´
            { DEADTRANS(L'A', 0xb4, 0xc1, 0x00) }, // letra A com ´
            { DEADTRANS(L'e', 0xb4, 0xe9, 0x00) }, // letra e com ´
            { DEADTRANS(L'E', 0xb4, 0xc9, 0x00) }, // letra E com ´
            { DEADTRANS(L'i', 0xb4, 0xed, 0x00) }, // letra i com ´
            { DEADTRANS(L'I', 0xb4, 0xcd, 0x00) }, // letra I com ´
            { DEADTRANS(L'o', 0xb4, 0xf3, 0x00) }, // letra o com ´
            { DEADTRANS(L'O', 0xb4, 0xd3, 0x00) }, // letra O com ´
            { DEADTRANS(L'u', 0xb4, 0xfa, 0x00) }, // letra u com ´
            { DEADTRANS(L'U', 0xb4, 0xda, 0x00) }, // letra U com ´
            { DEADTRANS(L'y', 0xb4, 0xfd, 0x00) }, // letra y com ´
            { DEADTRANS(0x20, 0xb4, 0xb4, 0x00) }, // letra 'space' (0x0020) com ´

            //*`* DEADKEY	0060
            { DEADTRANS(L'a', 0x60, 0xe0, 0x00) }, // letra a com `
            { DEADTRANS(L'A', 0x60, 0xc0, 0x00) }, // letra A com `
            { DEADTRANS(L'e', 0x60, 0xe8, 0x00) }, // letra e com `
            { DEADTRANS(L'E', 0x60, 0xc8, 0x00) }, // letra E com `
            { DEADTRANS(L'i', 0x60, 0xec, 0x00) }, // letra i com `
            { DEADTRANS(L'I', 0x60, 0xcc, 0x00) }, // letra I com `
            { DEADTRANS(L'o', 0x60, 0xf2, 0x00) }, // letra o com `
            { DEADTRANS(L'O', 0x60, 0xd2, 0x00) }, // letra O com `
            { DEADTRANS(L'u', 0x60, 0xf9, 0x00) }, // letra u com `
            { DEADTRANS(L'U', 0x60, 0xd9, 0x00) }, // letra U com `
            { DEADTRANS(0x20, 0x60, 0x60, 0x00) }, // letra 'space' (0x0020) com `

	    //*^* DEADKEY	005E
            { DEADTRANS(L'a', 0x5e, 0xe2, 0x00) }, // letra a com ^
            { DEADTRANS(L'A', 0x5e, 0xc2, 0x00) }, // letra A com ^
            { DEADTRANS(L'e', 0x5e, 0xea, 0x00) }, // letra e com ^
            { DEADTRANS(L'E', 0x5e, 0xca, 0x00) }, // letra E com ^
            { DEADTRANS(L'i', 0x5e, 0xee, 0x00) }, // letra i com ^
            { DEADTRANS(L'I', 0x5e, 0xce, 0x00) }, // letra I com ^
            { DEADTRANS(L'o', 0x5e, 0xf4, 0x00) }, // letra o com ^
            { DEADTRANS(L'O', 0x5e, 0xd4, 0x00) }, // letra O com ^
            { DEADTRANS(L'u', 0x5e, 0xfb, 0x00) }, // letra u com ^
            { DEADTRANS(L'U', 0x5e, 0xdb, 0x00) }, // letra U com ^
            { DEADTRANS(0x20, 0x5e, 0x5e, 0x00) }, // letra 'space' (0x0020) com ^

            //*~* DEADKEY	007E
            { DEADTRANS(L'a', 0x7e, 0xe3, 0x00) }, // letra a com ~
            { DEADTRANS(L'A', 0x7e, 0xc3, 0x00) }, // letra A com ~
            { DEADTRANS(L'n', 0x7e, 0xf1, 0x00) }, // letra e com ~
            { DEADTRANS(L'N', 0x7e, 0xd1, 0x00) }, // letra E com ~
            { DEADTRANS(L'o', 0x7e, 0xf5, 0x00) }, // letra i com ~
            { DEADTRANS(L'O', 0x7e, 0xd5, 0x00) }, // letra I com ~
            { DEADTRANS(0x20, 0x7e, 0x7e, 0x00) }, // letra 'space' (0x0020) com ~

            //*"* DEADKEY	00A8
            { DEADTRANS(L'a', 0xa8, 0xe4, 0x00) }, // letra a com "
            { DEADTRANS(L'A', 0xa8, 0xc4, 0x00) }, // letra A com "
            { DEADTRANS(L'e', 0xa8, 0xeb, 0x00) }, // letra e com "
            { DEADTRANS(L'E', 0xa8, 0xcb, 0x00) }, // letra E com "
            { DEADTRANS(L'i', 0xa8, 0xef, 0x00) }, // letra i com "
            { DEADTRANS(L'I', 0xa8, 0xcf, 0x00) }, // letra I com "
            { DEADTRANS(L'o', 0xa8, 0xf6, 0x00) }, // letra o com "
            { DEADTRANS(L'O', 0xa8, 0xd6, 0x00) }, // letra O com "
            { DEADTRANS(L'u', 0xa8, 0xfc, 0x00) }, // letra u com "
            { DEADTRANS(L'U', 0xa8, 0xdc, 0x00) }, // letra U com "
            { DEADTRANS(L'y', 0xa8, 0xff, 0x00) }, // letra y com "
            { DEADTRANS(0x20, 0xa8, 0xa8, 0x00) }, // letra 'space' (0x0020) com "

            { 0, 0, 0}
};






ROSDATA VSC_LPWSTR key_names[] = {
  { 0x01, L"Escape" },
  { 0x0e, L"BackSpace" },
  { 0x0f, L"Tab" },
  { 0x1c, L"Enter" },
  { 0x1d, L"Ctrl" },
  { 0x2a, L"Shift" },
  { 0x36, L"Shift Direito" },
  { 0x37, L"* Num" },
  { 0x38, L"Alt" },
  { 0x39, L"Espaco" },
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
  { 0x45, L"Pausa" },
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
  { 0x53, L"Del Num" },
  { 0x54, L"System Request" },
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

ROSDATA VSC_LPWSTR extended_key_names[] = {
  { 0x1c, L"Enter Num" },
  { 0x1d, L"Ctrl direito" },
  { 0x35, L"/ Num" },
  { 0x37, L"Print Screen" },
  { 0x38, L"Alt Direito" },
  { 0x45, L"Num Lock" },
  { 0x46, L"Pausa" },

  { 0x47, L"Home" },
  { 0x4f, L"End" },

  { 0x52, L"Insert" },
  { 0x53, L"Delete" },

  { 0x49, L"Page Up" },
  { 0x51, L"Page Down" },

  { 0x48, L"Cima" },
  { 0x50, L"Baixo" },
  { 0x4b, L"Esquerda" },
  { 0x4d, L"Direita" },

  { 0x54, L"<ReactOS>" },
  { 0x56, L"Ajuda" },
  { 0x5b, L"Windows Esquerda" },
  { 0x5c, L"Windows Direita" },
  { 0x5d, L"Aplicacao" },

  { 0, NULL },
};

ROSDATA DEADKEY_LPWSTR dead_key_names[] = {
    L"\x00b4"	L"Agudo",
    L"\x0060"	L"Grave",
    L"\x005e"	L"Circunflexo",
    L"\x007e"	L"Til",
    L"\x00a8"	L"Trema",
    NULL
};






/* Finally, the master table */
ROSDATA KBDTABLES keyboard_layout_table = {

  /* modifier assignments */
  &modifier_bits,

  /* character from vk tables */
  vk_to_wchar_master_table,

  /* diacritical marks -- currently implemented by wine code */
  /* Portuguese have severals */
  deadkey,

  /* Key names */
  (VSC_LPWSTR *)key_names,
  (VSC_LPWSTR *)extended_key_names,
  dead_key_names,
  /* Dead key names */

  /* scan code to virtual key maps */
  scancode_to_vk,
  sizeof(scancode_to_vk) / sizeof(scancode_to_vk[0]),
  extcode0_to_vk,
  extcode1_to_vk,

  MAKELONG(0,1), /* Version 1.0 */

  /* Ligatures -- Portuguese doesn't have any, that i'm aware  */
  0,
  0,

  NULL
};


PKBDTABLES STDCALL KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}
