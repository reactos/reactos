/*
 * ReactOS ESASCII Keyboard layout
 * Copyright (C) 2003 ReactOS
 * License: LGPL, see: LGPL.txt
 * Created by HUMA2000 from kbdus, kbdgr, kbdda and kbdfr
 * huma2000@terra.es
 * Thanks to arty for the kbtest utility and help
 * Thanks to carraca from reactos.com forum for his fixes
 * Thanks Elrond for help
 *
 * Thanks to: http://www.barcodeman.com/altek/mule/scandoc.php
 * and http://win.tue.nl/~aeb/linux/kbd/scancodes-1.html
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
/* 1a */  VK_OEM_1,
/* 1b */  VK_OEM_PLUS,
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

ROSDATA VSC_VK extcode0_to_vk[] = {
  { 0x10, VK_MEDIA_PREV_TRACK | KEXT }, // Pista anterior, no puedo probarlo hasta que no se implemente el sonido
  { 0x19, VK_MEDIA_NEXT_TRACK | KEXT }, // Pista siguiente,
  { 0x1D, VK_RCONTROL | KEXT }, // Tecla control
  { 0x20, VK_VOLUME_MUTE | KEXT }, // Silenciar volumen
  { 0x21, VK_LAUNCH_APP2 | KEXT }, // Tecla calculadora
  { 0x22, VK_MEDIA_PLAY_PAUSE | KEXT }, // Play/pause
  { 0x24, VK_MEDIA_STOP | KEXT }, // Stop
  { 0x2E, VK_VOLUME_DOWN | KEXT }, // Bajar volumen
  { 0x30, VK_VOLUME_UP | KEXT }, // Subir volumen
  { 0x32, VK_BROWSER_HOME | KEXT }, // Pagina de inicio del navegador de internet o abrirlo si no esta activolo
  { 0x35, VK_DIVIDE | KEXT }, // Tecla
  { 0x37, VK_SNAPSHOT | KEXT }, // La tecla de imprimir pantalla
  { 0x38, VK_RMENU | KEXT }, // Tecla alt
  { 0x47, VK_HOME | KEXT }, // Tecla inicio
  { 0x48, VK_UP | KEXT }, // Cursor arriba
  { 0x49, VK_PRIOR | KEXT }, // Tecla Re pag
  { 0x4B, VK_LEFT | KEXT }, // Cursor izquierda
  { 0x4D, VK_RIGHT | KEXT }, // Cursor derecha
  { 0x4F, VK_END | KEXT }, // Tecla Fin
  { 0x50, VK_DOWN | KEXT }, // Cursor abajo
  { 0x51, VK_NEXT | KEXT }, // Tecla Av pag
  { 0x52, VK_INSERT | KEXT }, // Tecla insertar
  { 0x53, VK_DELETE | KEXT }, // Tecla deletear
  { 0x5B, VK_LWIN | KEXT }, // Tecla windows izquierda
  { 0x5C, VK_RWIN | KEXT }, // Tecla windows derecha
  { 0x5D, VK_APPS | KEXT }, // Tecla menu aplicacion derecha
  { 0x5F, VK_SLEEP | KEXT }, // Tecla Sleep
  { 0x65, VK_BROWSER_SEARCH | KEXT }, // Pagina de b˙squeda en el navegador de internet
  { 0x66, VK_BROWSER_FAVORITES | KEXT }, // Favoritos, tengo que esperar a que el tcp/ip
  { 0x67, VK_BROWSER_REFRESH | KEXT }, // Refrescar el navegador de internet
  { 0x68, VK_BROWSER_STOP | KEXT }, // Stop en el navegador de internet
  { 0x69, VK_BROWSER_FORWARD | KEXT }, // Adelante en el navegador de internet
  { 0x6A, VK_BROWSER_BACK | KEXT }, // Atr·s en el navegador de internet
  { 0x6B, VK_LAUNCH_APP1 | KEXT }, // Tecla Mi pc
  { 0x6C, VK_LAUNCH_MAIL | KEXT }, // Abrir programa de e-mail
  { 0x6D, VK_LAUNCH_MEDIA_SELECT | KEXT }, // Abrir reproductor multimedia
  { 0x1C, VK_RETURN | KEXT }, // La tecla de intro
  { 0x46, VK_CANCEL | KEXT }, // Tecla escape
  { 0, 0 },
};


ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0, 0 },
};

ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT,   KSHIFT },
  { VK_CONTROL, KCTRL },
  { VK_MENU,    KALT },
  { 0,  0 }
};

ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  6,
  {   0,     1,    2,          4,   SHFT_INVALID, SHFT_INVALID, 3  }
/* NONE, SHIFT, CTRL, CTRL+SHIFT, ALT */
};

#define NOCAPS 0
#define CAPS   KSHIFT /* Caps -> shift */

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  /* Normal vs Shifted */
  /* The numbers */
  //Del 1 al 5 tienen tres estados
  { '4',         NOCAPS, {'4', '$'} },
  { '5', 	     NOCAPS, {'5',	'%'} },
  //El 6 tiene 3 estados
  { '7',         NOCAPS, {'7', '/'} },
  { '8',         NOCAPS, {'8', '('} },
  { '9',         NOCAPS, {'9', ')'} },
  { '0',         NOCAPS, {'0', '='} },
  { VK_OEM_3,    CAPS,   {0x00F1, 0x00D1} }, // Ò—

  /* Specials */
  /* Ctrl-_ generates ES */
  { VK_OEM_6	 ,NOCAPS, {0x00a1, 0x00bf} }, // °ø
  { VK_OEM_4       ,NOCAPS, {0x0027, '?'}    }, //'?
  { VK_OEM_COMMA   ,NOCAPS, {',',    ';'}    },
  { VK_OEM_PERIOD  ,NOCAPS, {'.',    ':'}    },
  { VK_OEM_MINUS   ,NOCAPS, {'-',    '_'}    },
  { VK_OEM_102     ,NOCAPS, {'<',    '>'}    },

  /* Keys that do not have shift states */
  { VK_TAB,		NOCAPS, {'\t',	'\t'}   },
  { VK_ADD,		NOCAPS, {'+', 	'+'}    },
  { VK_SUBTRACT,  NOCAPS, {'-', 	'-'}    },
  { VK_MULTIPLY,	NOCAPS, {'*', 	'*'}    },
  { VK_DIVIDE,	NOCAPS, {'/', 	'/'}    },
  { VK_ESCAPE,	NOCAPS, {'\x1b','\x1b'} },
  { VK_SPACE,	NOCAPS, {' ', 	' '}    },
  { 0, 0 }
};


ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  /* Normal, Shifted, Ctrl */

  /* The alphabet */
  { 'A',         CAPS,   {'a', 'A', 0x01} },
  { 'B',         CAPS,   {'b', 'B', 0x02} },
  { 'C',         CAPS,   {'c', 'C', 0x03} },
  { 'D',         CAPS,   {'d', 'D', 0x04} },
  { 'F',         CAPS,   {'f', 'F', 0x06} },
  { 'G',         CAPS,   {'g', 'G', 0x07} },
  { 'H',         CAPS,   {'h', 'H', 0x08} },
  { 'I',         CAPS,   {'i', 'I', 0x09} },
  { 'J',         CAPS,   {'j', 'J', 0x0a} },
  { 'K',         CAPS,   {'k', 'K', 0x0b} },
  { 'L',         CAPS,   {'l', 'L', 0x0c} },
  { 'M',         CAPS,   {'m', 'M', 0x0d} },
  { 'N',         CAPS,   {'n', 'N', 0x0e} },
  { 'O',         CAPS,   {'o', 'O', 0x0f} },
  { 'P',         CAPS,   {'p', 'P', 0x10} },
  { 'Q',         CAPS,   {'q', 'Q', 0x11} },
  { 'R',         CAPS,   {'r', 'R', 0x12} },
  { 'S',         CAPS,   {'s', 'S', 0x13} },
  { 'T',         CAPS,   {'t', 'T', 0x14} },
  { 'U',         CAPS,   {'u', 'U', 0x15} },
  { 'V',         CAPS,   {'v', 'V', 0x16} },
  { 'W',         CAPS,   {'w', 'W', 0x17} },
  { 'X',         CAPS,   {'x', 'X', 0x18} },
  { 'Y',         CAPS,   {'y', 'Y', 0x19} },
  { 'Z',         CAPS,   {'z', 'Z', 0x1a} },

  /* Legacy (telnet-style) ascii escapes */
  { VK_RETURN, 	NOCAPS, {'\r',     '\r',     '\n'}    },
  { 0,0 }
};


ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
/* Normal, shifted, control, Alt+Gr */
  { '1', 		NOCAPS, {'1',      '!',      WCH_NONE,  0x00a6}  }, // 1!|
  { '2', 		NOCAPS, {'2',  	'"',      WCH_NONE,  '@'}     }, // 2"@
  { '3', 		NOCAPS, {'3', 	0x00B7,   WCH_NONE,  '#'}     }, // 3∑#
  { '6', 		NOCAPS, {'6', 	'&',      WCH_NONE,  0x00AC}  }, // 6&¨
  { 'E', 		CAPS,   {'e', 	'E',      0x05,  0x20AC}  }, // eEÄ
  { VK_OEM_PLUS,  NOCAPS, {'+',      '*', 	    WCH_NONE,  0x005d}  }, // +*]
  { VK_OEM_2,  	NOCAPS, {0x00e7,   0x00c7,   WCH_NONE,  '}'}     }, // Á«}

  { VK_OEM_7,  	NOCAPS, {WCH_DEAD,   WCH_DEAD, WCH_NONE,  '{'} }, //  ¥®{
  { VK_EMPTY, 	NOCAPS, {0xB4,       0xA8,     WCH_NONE,  WCH_NONE} },  //  ¥®{

  { VK_OEM_1,  	NOCAPS, {WCH_DEAD,   WCH_DEAD, WCH_NONE,  0x5B}    }, // `^[
  { VK_EMPTY,  	NOCAPS, {0x60,       0x5e,     WCH_NONE,  WCH_NONE}  }, // `^[

  { VK_OEM_5,  	NOCAPS, {0x00BA,   0x00AA,   WCH_NONE,  0x005c}  }, // Á«}
  { 0, 0 }
};



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
  { VK_BACK,    0, {'\010'} },
  { 0,0 }
};


#define vk_master(n,x) { (PVK_TO_WCHARS1)x, n, sizeof(x[0]) }


ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(1,keypad_numbers),
  vk_master(2,key_to_chars_2mod),
  vk_master(3,key_to_chars_3mod),
  vk_master(4,key_to_chars_4mod),
  { 0,0,0 }
};

#define DEADTRANS(ch, accent, comp, flags) MAKELONG(ch, accent), comp, flags //no funciona
ROSDATA DEADKEY  deadkey[] =
{
            //*¥*
            { DEADTRANS(L'a', 0xb4, 0xE1, 0x00) }, // letra a con ¥
            { DEADTRANS(L'A', 0xb4, 0xC1, 0x00) }, // letra A con ¥
            { DEADTRANS(L'e', 0xb4, 0xE9, 0x00) }, // letra e con ¥
            { DEADTRANS(L'E', 0xb4, 0xC9, 0x00) }, // letra E con ¥
            { DEADTRANS(L'i', 0xb4, 0xED, 0x00) }, // letra i con ¥
            { DEADTRANS(L'I', 0xb4, 0xCD, 0x00) }, // letra I con ¥
            { DEADTRANS(L'o', 0xb4, 0xF3, 0x00) }, // letra o con ¥
            { DEADTRANS(L'O', 0xb4, 0xD3, 0x00) }, // letra O con ¥
            { DEADTRANS(L'u', 0xb4, 0xFA, 0x00) }, // letra u con ¥
            { DEADTRANS(L'U', 0xb4, 0xDA, 0x00) }, // letra U con ¥
            //*`*
            { DEADTRANS(L'a', 0x60, 0xE0, 0x00) }, // letra a con `
            { DEADTRANS(L'A', 0x60, 0xC0, 0x00) }, // letra A con `
            { DEADTRANS(L'e', 0x60, 0xE8, 0x00) }, // letra e con `
            { DEADTRANS(L'E', 0x60, 0xC8, 0x00) }, // letra E con `
            { DEADTRANS(L'i', 0x60, 0xEC, 0x00) }, // letra i con `
            { DEADTRANS(L'I', 0x60, 0xCC, 0x00) }, // letra I con `
            { DEADTRANS(L'o', 0x60, 0xF2, 0x00) }, // letra o con `
            { DEADTRANS(L'O', 0x60, 0xD2, 0x00) }, // letra O con `
            { DEADTRANS(L'u', 0x60, 0xF9, 0x00) }, // letra u con `
            { DEADTRANS(L'U', 0x60, 0xD9, 0x00) }, // letra U con `
            //*^*
            { DEADTRANS(L'a', 0x5E, 0xE2, 0x00) }, // letra a con ^
            { DEADTRANS(L'A', 0x5E, 0xC2, 0x00) }, // letra A con ^
            { DEADTRANS(L'e', 0x5E, 0xEA, 0x00) }, // letra e con ^
            { DEADTRANS(L'E', 0x5E, 0xCA, 0x00) }, // letra E con ^
            { DEADTRANS(L'i', 0x5E, 0xEE, 0x00) }, // letra i con ^
            { DEADTRANS(L'I', 0x5E, 0xCE, 0x00) }, // letra I con ^
            { DEADTRANS(L'o', 0x5E, 0xF4, 0x00) }, // letra o con ^
            { DEADTRANS(L'O', 0x5E, 0xD4, 0x00) }, // letra O con ^
            { DEADTRANS(L'u', 0x5E, 0xFB, 0x00) }, // letra u con ^
            { DEADTRANS(L'U', 0x5E, 0xDB, 0x00) }, // letra U con ^
            //*®*
            { DEADTRANS(L'a', 0xA8, 0xE4, 0x00) }, // letra a con ®
            { DEADTRANS(L'A', 0xA8, 0xC4, 0x00) }, // letra A con ®
            { DEADTRANS(L'e', 0xA8, 0xEB, 0x00) }, // letra e con ®
            { DEADTRANS(L'E', 0xA8, 0xCB, 0x00) }, // letra E con ®
            { DEADTRANS(L'i', 0xA8, 0xEF, 0x00) }, // letra i con ®
            { DEADTRANS(L'I', 0xA8, 0xCF, 0x00) }, // letra I con ®
            { DEADTRANS(L'o', 0xA8, 0xF6, 0x00) }, // letra o con ®
            { DEADTRANS(L'O', 0xA8, 0xD6, 0x00) }, // letra O con ®
            { DEADTRANS(L'u', 0xA8, 0xFC, 0x00) }, // letra u con ®
            { DEADTRANS(L'U', 0xA8, 0xDC, 0x00) }, // letra U con ®
            { 0, 0, 0}
};

ROSDATA VSC_LPWSTR key_names[] = {
  { 0x00, L"" },
  { 0x01, L"Escape" },
  { 0x0e, L"Borrar" },
  { 0x0f, L"Tabulador" },
  { 0x1c, L"Intro" },
  { 0x1d, L"Ctrl" },
  { 0x2a, L"Shift" },
  { 0x36, L"Shift derecho" },
  { 0x37, L"* numerico" },
  { 0x38, L"Alt" },
  { 0x39, L"Espacio" },
  { 0x3a, L"Bloqueo mayusculas" },
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
  { 0x46, L"Bloqueo de scroll" },
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
  { 0x53, L"Borrardo numerico" },
  { 0x54, L"Peticion de sistema" },
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
  { 0x1c, L"Intro numerico" },
  { 0x1d, L"Ctrl derecho" },
  { 0x35, L"/ numerica" },
  { 0x37, L"Imprimir pantalla" },
  { 0x38, L"Alt derecho" },
  { 0x45, L"Bloqueo numerico" },
  { 0x46, L"Interrumpir" },
  { 0x47, L"Inicio" },
  { 0x48, L"Arriba" },
  { 0x49, L"Subir pagina" },
  { 0x4b, L"Izquierda" },
  { 0x4c, L"Centrar" },
  { 0x4d, L"Derecha" },
  { 0x4f, L"Fin" },
  { 0x50, L"Abajo" },
  { 0x51, L"Bajar pagina" },
  { 0x52, L"Insertar" },
  { 0x53, L"Borrar" },
  { 0x54, L"<ReactOS>" },
  { 0x55, L"Ayuda" },
  { 0x5b, L"Windows izquierda" },
  { 0x5c, L"Windows derecha" },
  { 0, NULL },
};

ROSDATA DEADKEY_LPWSTR dead_key_names[] = {
    L"\x00b4"	L"Agudo",
    L"\x0060"	L"Grave",
    L"\x005e"	L"Circunflejo",
	L"\x00A8"	L"Dieresis",
    NULL
};


/* Finally, the master table */
ROSDATA KBDTABLES keyboard_layout_table = {

  /* modifier assignments */
  &modifier_bits,

  /* character from vk tables */
  vk_to_wchar_master_table,

  /* diacritical marks -- currently implemented by wine code */
  /* Spanish have severals */
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

  /* Ligatures -- Spanish doesn't have any  */
  0,
  0,
  NULL
};


PKBDTABLES STDCALL KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}
