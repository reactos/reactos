/*
 * Copyright (C) the Wine project
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

/* Macro to deal with LP64 <=> LLP64 differences in numeric constants with 'l' modifier */
#ifndef __MSABI_LONG
# if defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__)
#  define __MSABI_LONG(x)         x ## l
# else
#  define __MSABI_LONG(x)         x
# endif
#endif

#ifdef RC_INVOKED
# define MAKEINTRESOURCE(i)       i
#endif


#define RT_MANIFEST                                        MAKEINTRESOURCE(24)
#define CREATEPROCESS_MANIFEST_RESOURCE_ID                 MAKEINTRESOURCE(1)
#define ISOLATIONAWARE_MANIFEST_RESOURCE_ID                MAKEINTRESOURCE(2)
#define ISOLATIONAWARE_NOSTATICIMPORT_MANIFEST_RESOURCE_ID MAKEINTRESOURCE(3)
#define MINIMUM_RESERVED_MANIFEST_RESOURCE_ID              MAKEINTRESOURCE(1)
#define MAXIMUM_RESERVED_MANIFEST_RESOURCE_ID              MAKEINTRESOURCE(16)


/*** ShowWindow() codes ***/
#define SW_HIDE                0
#define SW_SHOWNORMAL          1
#define SW_NORMAL              SW_SHOWNORMAL
#define SW_SHOWMINIMIZED       2
#define SW_SHOWMAXIMIZED       3
#define SW_MAXIMIZE            SW_SHOWMAXIMIZED
#define SW_SHOWNOACTIVATE      4
#define SW_SHOW                5
#define SW_MINIMIZE            6
#define SW_SHOWMINNOACTIVE     7
#define SW_SHOWNA              8
#define SW_RESTORE             9
#define SW_SHOWDEFAULT         10
#define SW_FORCEMINIMIZE       11
#define SW_MAX                 11
#define SW_NORMALNA            0xCC /* Undocumented. Flag in MinMaximize */

/* Obsolete ShowWindow() codes for compatibility */
#define HIDE_WINDOW            SW_HIDE
#define SHOW_OPENWINDOW        SW_SHOWNORMAL
#define SHOW_ICONWINDOW        SW_SHOWMINIMIZED
#define SHOW_FULLSCREEN        SW_SHOWMAXIMIZED
#define SHOW_OPENNOACTIVATE    SW_SHOWNOACTIVATE

/* WM_SHOWWINDOW lParam codes */
#define SW_PARENTCLOSING       1
#define SW_OTHERZOOM           2
#define SW_PARENTOPENING       3
#define SW_OTHERUNZOOM         4


/*** Virtual key codes ***/
#define VK_LBUTTON             0x01
#define VK_RBUTTON             0x02
#define VK_CANCEL              0x03
#define VK_MBUTTON             0x04
#define VK_XBUTTON1            0x05
#define VK_XBUTTON2            0x06
/*                             0x07  Undefined */
#define VK_BACK                0x08
#define VK_TAB                 0x09
/*                             0x0A-0x0B  Undefined */
#define VK_CLEAR               0x0C
#define VK_RETURN              0x0D
/*                             0x0E-0x0F  Undefined */
#define VK_SHIFT               0x10
#define VK_CONTROL             0x11
#define VK_MENU                0x12
#define VK_PAUSE               0x13
#define VK_CAPITAL             0x14

#define VK_KANA                0x15
#define VK_HANGEUL             VK_KANA
#define VK_HANGUL              VK_KANA
/*                             0x16  Undefined */
#define VK_JUNJA               0x17
#define VK_FINAL               0x18
#define VK_HANJA               0x19
#define VK_KANJI               VK_HANJA

/*                             0x1A       Undefined */
#define VK_ESCAPE              0x1B

#define VK_CONVERT             0x1C
#define VK_NONCONVERT          0x1D
#define VK_ACCEPT              0x1E
#define VK_MODECHANGE          0x1F

#define VK_SPACE               0x20
#define VK_PRIOR               0x21
#define VK_NEXT                0x22
#define VK_END                 0x23
#define VK_HOME                0x24
#define VK_LEFT                0x25
#define VK_UP                  0x26
#define VK_RIGHT               0x27
#define VK_DOWN                0x28
#define VK_SELECT              0x29
#define VK_PRINT               0x2A /* OEM specific in Windows 3.1 SDK */
#define VK_EXECUTE             0x2B
#define VK_SNAPSHOT            0x2C
#define VK_INSERT              0x2D
#define VK_DELETE              0x2E
#define VK_HELP                0x2F
/* VK_0 - VK-9                 0x30-0x39  Use ASCII instead */
/*                             0x3A-0x40  Undefined */
/* VK_A - VK_Z                 0x41-0x5A  Use ASCII instead */
#define VK_LWIN                0x5B
#define VK_RWIN                0x5C
#define VK_APPS                0x5D
/*                             0x5E Unassigned */
#define VK_SLEEP               0x5F
#define VK_NUMPAD0             0x60
#define VK_NUMPAD1             0x61
#define VK_NUMPAD2             0x62
#define VK_NUMPAD3             0x63
#define VK_NUMPAD4             0x64
#define VK_NUMPAD5             0x65
#define VK_NUMPAD6             0x66
#define VK_NUMPAD7             0x67
#define VK_NUMPAD8             0x68
#define VK_NUMPAD9             0x69
#define VK_MULTIPLY            0x6A
#define VK_ADD                 0x6B
#define VK_SEPARATOR           0x6C
#define VK_SUBTRACT            0x6D
#define VK_DECIMAL             0x6E
#define VK_DIVIDE              0x6F
#define VK_F1                  0x70
#define VK_F2                  0x71
#define VK_F3                  0x72
#define VK_F4                  0x73
#define VK_F5                  0x74
#define VK_F6                  0x75
#define VK_F7                  0x76
#define VK_F8                  0x77
#define VK_F9                  0x78
#define VK_F10                 0x79
#define VK_F11                 0x7A
#define VK_F12                 0x7B
#define VK_F13                 0x7C
#define VK_F14                 0x7D
#define VK_F15                 0x7E
#define VK_F16                 0x7F
#define VK_F17                 0x80
#define VK_F18                 0x81
#define VK_F19                 0x82
#define VK_F20                 0x83
#define VK_F21                 0x84
#define VK_F22                 0x85
#define VK_F23                 0x86
#define VK_F24                 0x87
#define VK_NAVIGATION_VIEW     0x88
#define VK_NAVIGATION_MENU     0x89
#define VK_NAVIGATION_UP       0x8A
#define VK_NAVIGATION_DOWN     0x8B
#define VK_NAVIGATION_LEFT     0x8C
#define VK_NAVIGATION_RIGHT    0x8D
#define VK_NAVIGATION_ACCEPT   0x8E
#define VK_NAVIGATION_CANCEL   0x8F
#define VK_NUMLOCK             0x90
#define VK_SCROLL              0x91
#define VK_OEM_NEC_EQUAL       0x92
#define VK_OEM_FJ_JISHO        0x92
#define VK_OEM_FJ_MASSHOU      0x93
#define VK_OEM_FJ_TOUROKU      0x94
#define VK_OEM_FJ_LOYA         0x95
#define VK_OEM_FJ_ROYA         0x96
/*                             0x97-0x9F  Unassigned */
/*
 * differencing between right and left shift/control/alt key.
 * Used only by GetAsyncKeyState() and GetKeyState().
 */
#define VK_LSHIFT              0xA0
#define VK_RSHIFT              0xA1
#define VK_LCONTROL            0xA2
#define VK_RCONTROL            0xA3
#define VK_LMENU               0xA4
#define VK_RMENU               0xA5

#define VK_BROWSER_BACK        0xA6
#define VK_BROWSER_FORWARD     0xA7
#define VK_BROWSER_REFRESH     0xA8
#define VK_BROWSER_STOP        0xA9
#define VK_BROWSER_SEARCH      0xAA
#define VK_BROWSER_FAVORITES   0xAB
#define VK_BROWSER_HOME        0xAC
#define VK_VOLUME_MUTE         0xAD
#define VK_VOLUME_DOWN         0xAE
#define VK_VOLUME_UP           0xAF
#define VK_MEDIA_NEXT_TRACK    0xB0
#define VK_MEDIA_PREV_TRACK    0xB1
#define VK_MEDIA_STOP          0xB2
#define VK_MEDIA_PLAY_PAUSE    0xB3
#define VK_LAUNCH_MAIL         0xB4
#define VK_LAUNCH_MEDIA_SELECT 0xB5
#define VK_LAUNCH_APP1         0xB6
#define VK_LAUNCH_APP2         0xB7

/*                             0xB8-0xB9  Unassigned */
#define VK_OEM_1               0xBA
#define VK_OEM_PLUS            0xBB
#define VK_OEM_COMMA           0xBC
#define VK_OEM_MINUS           0xBD
#define VK_OEM_PERIOD          0xBE
#define VK_OEM_2               0xBF
#define VK_OEM_3               0xC0
/*                             0xC1-0xC2  Unassigned */
#define VK_GAMEPAD_A           0xC3
#define VK_GAMEPAD_B           0xC4
#define VK_GAMEPAD_X           0xC5
#define VK_GAMEPAD_Y           0xC6
#define VK_GAMEPAD_RIGHT_SHOULDER 0xC7
#define VK_GAMEPAD_LEFT_SHOULDER 0xC8
#define VK_GAMEPAD_LEFT_TRIGGER 0xC9
#define VK_GAMEPAD_RIGHT_TRIGGER 0xCA
#define VK_GAMEPAD_DPAD_UP     0xCB
#define VK_GAMEPAD_DPAD_DOWN   0xCC
#define VK_GAMEPAD_DPAD_LEFT   0xCD
#define VK_GAMEPAD_DPAD_RIGHT  0xCE
#define VK_GAMEPAD_MENU        0xCF
#define VK_GAMEPAD_VIEW        0xD0
#define VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON 0xD1
#define VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON 0xD2
#define VK_GAMEPAD_LEFT_THUMBSTICK_UP 0xD3
#define VK_GAMEPAD_LEFT_THUMBSTICK_DOWN 0xD4
#define VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT 0xD5
#define VK_GAMEPAD_LEFT_THUMBSTICK_LEFT 0xD6
#define VK_GAMEPAD_RIGHT_THUMBSTICK_UP 0xD7
#define VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN 0xD8
#define VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT 0xD9
#define VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT 0xDA
#define VK_OEM_4               0xDB
#define VK_OEM_5               0xDC
#define VK_OEM_6               0xDD
#define VK_OEM_7               0xDE
#define VK_OEM_8               0xDF
/*                             0xE0       OEM specific */
#define VK_OEM_AX              0xE1  /* "AX" key on Japanese AX keyboard */
#define VK_OEM_102             0xE2  /* "<>" or "\|" on RT 102-key keyboard */
#define VK_ICO_HELP            0xE3  /* Help key on ICO */
#define VK_ICO_00              0xE4  /* 00 key on ICO */
#define VK_PROCESSKEY          0xE5
#define VK_ICO_CLEAR           0xE6

#define VK_PACKET              0xE7
/*                             0xE8       Unassigned */

#define VK_OEM_RESET           0xE9
#define VK_OEM_JUMP            0xEA
#define VK_OEM_PA1             0xEB
#define VK_OEM_PA2             0xEC
#define VK_OEM_PA3             0xED
#define VK_OEM_WSCTRL          0xEE
#define VK_OEM_CUSEL           0xEF
#define VK_OEM_ATTN            0xF0
#define VK_OEM_FINISH          0xF1
#define VK_OEM_COPY            0xF2
#define VK_OEM_AUTO            0xF3
#define VK_OEM_ENLW            0xF4
#define VK_OEM_BACKTAB         0xF5
#define VK_ATTN                0xF6
#define VK_CRSEL               0xF7
#define VK_EXSEL               0xF8
#define VK_EREOF               0xF9
#define VK_PLAY                0xFA
#define VK_ZOOM                0xFB
#define VK_NONAME              0xFC
#define VK_PA1                 0xFD
#define VK_OEM_CLEAR           0xFE
/*                             0xFF       Unassigned */


/*** Messages ***/
#ifndef NOWINMESSAGES
#define WM_NULL                0x0000
#define WM_CREATE              0x0001
#define WM_DESTROY             0x0002
#define WM_MOVE                0x0003
#define WM_SIZEWAIT            0x0004 /* DDK / Win16 */
#define WM_SIZE                0x0005
#define WM_ACTIVATE            0x0006

/* WM_ACTIVATE wParam values */
#define WA_INACTIVE            0
#define WA_ACTIVE              1
#define WA_CLICKACTIVE         2

#define WM_SETFOCUS            0x0007
#define WM_KILLFOCUS           0x0008
#define WM_SETVISIBLE          0x0009 /* DDK / Win16 */
#define WM_ENABLE              0x000a
#define WM_SETREDRAW           0x000b
#define WM_SETTEXT             0x000c
#define WM_GETTEXT             0x000d
#define WM_GETTEXTLENGTH       0x000e
#define WM_PAINT               0x000f
#define WM_CLOSE               0x0010
#define WM_QUERYENDSESSION     0x0011
#define WM_QUIT                0x0012
#define WM_QUERYOPEN           0x0013
#define WM_ERASEBKGND          0x0014
#define WM_SYSCOLORCHANGE      0x0015
#define WM_ENDSESSION          0x0016
#define WM_SYSTEMERROR         0x0017 /* DDK / Win16 */
#define WM_SHOWWINDOW          0x0018
#define WM_CTLCOLOR            0x0019 /* Added from windowsx.h */
#define WM_WININICHANGE        0x001a
#define WM_SETTINGCHANGE       WM_WININICHANGE
#define WM_DEVMODECHANGE       0x001b
#define WM_ACTIVATEAPP         0x001c
#define WM_FONTCHANGE          0x001d
#define WM_TIMECHANGE          0x001e
#define WM_CANCELMODE          0x001f
#define WM_SETCURSOR           0x0020
#define WM_MOUSEACTIVATE       0x0021
#define WM_CHILDACTIVATE       0x0022
#define WM_QUEUESYNC           0x0023
#define WM_GETMINMAXINFO       0x0024

#define WM_PAINTICON           0x0026
#define WM_ICONERASEBKGND      0x0027
#define WM_NEXTDLGCTL          0x0028
#define WM_ALTTABACTIVE        0x0029 /* DDK / Win16 */
#define WM_SPOOLERSTATUS       0x002a
#define WM_DRAWITEM            0x002b
#define WM_MEASUREITEM         0x002c
#define WM_DELETEITEM          0x002d
#define WM_VKEYTOITEM          0x002e
#define WM_CHARTOITEM          0x002f
#define WM_SETFONT             0x0030
#define WM_GETFONT             0x0031
#define WM_SETHOTKEY           0x0032
#define WM_GETHOTKEY           0x0033
#define WM_FILESYSCHANGE       0x0034 /* DDK / Win16 */
#define WM_ISACTIVEICON        0x0035 /* DDK / Win16 */
#define WM_QUERYPARKICON       0x0036 /* Undocumented */
#define WM_QUERYDRAGICON       0x0037
#define WM_QUERYSAVESTATE      0x0038 /* Undocumented */
#define WM_COMPAREITEM         0x0039
#define WM_TESTING             0x003a /* DDK / Win16 */

#define WM_GETOBJECT           0x003d

#define WM_ACTIVATESHELLWINDOW 0x003e /* FIXME: Wine-only */

#define WM_COMPACTING          0x0041

#define WM_COMMNOTIFY          0x0044
#define WM_WINDOWPOSCHANGING   0x0046
#define WM_WINDOWPOSCHANGED    0x0047

#define WM_POWER               0x0048

/* For WM_POWER */
#define PWR_OK                 1
#define PWR_FAIL               (-1)
#define PWR_SUSPENDREQUEST     1
#define PWR_SUSPENDRESUME      2
#define PWR_CRITICALRESUME     3

/* Win32 4.0 messages */
#define WM_COPYDATA            0x004a
#define WM_CANCELJOURNAL       0x004b
#define WM_KEYF1               0x004d /* DDK / Win16 */
#define WM_NOTIFY              0x004e
#define WM_INPUTLANGCHANGEREQUEST 0x0050
#define WM_INPUTLANGCHANGE     0x0051
#define WM_TCARD               0x0052
#define WM_HELP                0x0053
#define WM_USERCHANGED         0x0054
#define WM_NOTIFYFORMAT        0x0055

/* WM_NOTIFYFORMAT commands and return values */
#define NFR_ANSI               1
#define NFR_UNICODE            2
#define NF_QUERY               3
#define NF_REQUERY             4

#define WM_CONTEXTMENU         0x007b
#define WM_STYLECHANGING       0x007c
#define WM_STYLECHANGED        0x007d
#define WM_DISPLAYCHANGE       0x007e
#define WM_GETICON             0x007f
#define WM_SETICON             0x0080

/* Non-client system messages */
#define WM_NCCREATE            0x0081
#define WM_NCDESTROY           0x0082
#define WM_NCCALCSIZE          0x0083
#define WM_NCHITTEST           0x0084
#define WM_NCPAINT             0x0085
#define WM_NCACTIVATE          0x0086

#define WM_GETDLGCODE          0x0087
#define WM_SYNCPAINT           0x0088
#define WM_SYNCTASK            0x0089 /* DDK / Win16 */

/* Non-client mouse messages */
#define WM_NCMOUSEMOVE         0x00a0
#define WM_NCLBUTTONDOWN       0x00a1
#define WM_NCLBUTTONUP         0x00a2
#define WM_NCLBUTTONDBLCLK     0x00a3
#define WM_NCRBUTTONDOWN       0x00a4
#define WM_NCRBUTTONUP         0x00a5
#define WM_NCRBUTTONDBLCLK     0x00a6
#define WM_NCMBUTTONDOWN       0x00a7
#define WM_NCMBUTTONUP         0x00a8
#define WM_NCMBUTTONDBLCLK     0x00a9

#define WM_NCXBUTTONDOWN       0x00ab
#define WM_NCXBUTTONUP         0x00ac
#define WM_NCXBUTTONDBLCLK     0x00ad

/* Raw input */
#define WM_INPUT_DEVICE_CHANGE 0x00fe
#define WM_INPUT               0x00ff

/* Keyboard messages */
#define WM_KEYFIRST            0x0100
#define WM_KEYDOWN             WM_KEYFIRST
#define WM_KEYUP               0x0101
#define WM_CHAR                0x0102
#define WM_DEADCHAR            0x0103
#define WM_SYSKEYDOWN          0x0104
#define WM_SYSKEYUP            0x0105
#define WM_SYSCHAR             0x0106
#define WM_SYSDEADCHAR         0x0107
#define WM_UNICHAR             0x0109
#define WM_KEYLAST             WM_UNICHAR

#define UNICODE_NOCHAR         0xffff

/* Win32 4.0 messages for IME */
#define WM_IME_STARTCOMPOSITION 0x010d
#define WM_IME_ENDCOMPOSITION  0x010e
#define WM_IME_COMPOSITION     0x010f
#define WM_IME_KEYLAST         0x010f

#define WM_INITDIALOG          0x0110
#define WM_COMMAND             0x0111
#define WM_SYSCOMMAND          0x0112
#define WM_TIMER               0x0113

/* Scroll messages */
#define WM_HSCROLL             0x0114
#define WM_VSCROLL             0x0115

/* Menu messages */
#define WM_INITMENU            0x0116
#define WM_INITMENUPOPUP       0x0117
#define WM_GESTURE             0x0119
#define WM_GESTURENOTIFY       0x011A

#define WM_MENUSELECT          0x011F
#define WM_MENUCHAR            0x0120
#define WM_ENTERIDLE           0x0121

#define WM_MENURBUTTONUP       0x0122
#define WM_MENUDRAG            0x0123
#define WM_MENUGETOBJECT       0x0124
#define WM_UNINITMENUPOPUP     0x0125
#define WM_MENUCOMMAND         0x0126

#define WM_CHANGEUISTATE       0x0127
#define WM_UPDATEUISTATE       0x0128
#define WM_QUERYUISTATE        0x0129

/* UI flags for WM_*UISTATE */
/* for low-order word of wparam */
#define UIS_SET                1
#define UIS_CLEAR              2
#define UIS_INITIALIZE         3
/* for hi-order word of wparam */
#define UISF_HIDEFOCUS         0x1
#define UISF_HIDEACCEL         0x2
#define UISF_ACTIVE            0x4

#define WM_LBTRACKPOINT        0x0131 /* DDK / Win16 */

/* Win32 CTLCOLOR messages */
#define WM_CTLCOLORMSGBOX      0x0132
#define WM_CTLCOLOREDIT        0x0133
#define WM_CTLCOLORLISTBOX     0x0134
#define WM_CTLCOLORBTN         0x0135
#define WM_CTLCOLORDLG         0x0136
#define WM_CTLCOLORSCROLLBAR   0x0137
#define WM_CTLCOLORSTATIC      0x0138

#define MN_GETHMENU            0x01E1

/* Mouse messages */
#define WM_MOUSEFIRST          0x0200
#define WM_MOUSEMOVE           WM_MOUSEFIRST
#define WM_LBUTTONDOWN         0x0201
#define WM_LBUTTONUP           0x0202
#define WM_LBUTTONDBLCLK       0x0203
#define WM_RBUTTONDOWN         0x0204
#define WM_RBUTTONUP           0x0205
#define WM_RBUTTONDBLCLK       0x0206
#define WM_MBUTTONDOWN         0x0207
#define WM_MBUTTONUP           0x0208
#define WM_MBUTTONDBLCLK       0x0209
#define WM_MOUSEWHEEL          0x020A
#define WM_XBUTTONDOWN         0x020B
#define WM_XBUTTONUP           0x020C
#define WM_XBUTTONDBLCLK       0x020D
#define WM_MOUSEHWHEEL         0x020E
#define WM_MOUSELAST           WM_MOUSEHWHEEL

/* Macros for the mouse messages */
#define WHEEL_DELTA            120
#define GET_WHEEL_DELTA_WPARAM(wParam) ((short)HIWORD(wParam))
#define WHEEL_PAGESCROLL       (UINT_MAX)

#define GET_KEYSTATE_WPARAM(wParam)     (LOWORD(wParam))
#define GET_NCHITTEST_WPARAM(wParam)    ((short)LOWORD(wParam))
#define GET_XBUTTON_WPARAM(wParam)      (HIWORD(wParam))
#define XBUTTON1               0x0001
#define XBUTTON2               0x0002

#define WM_PARENTNOTIFY        0x0210
#define WM_ENTERMENULOOP       0x0211
#define WM_EXITMENULOOP        0x0212
#define WM_NEXTMENU            0x0213

/* Win32 4.0 messages */
#define WM_SIZING              0x0214
#define WM_CAPTURECHANGED      0x0215
#define WM_MOVING              0x0216
#define WM_POWERBROADCAST      0x0218
#define WM_DEVICECHANGE        0x0219
/* MDI messages */
#define WM_MDICREATE           0x0220
#define WM_MDIDESTROY          0x0221
#define WM_MDIACTIVATE         0x0222
#define WM_MDIRESTORE          0x0223
#define WM_MDINEXT             0x0224
#define WM_MDIMAXIMIZE         0x0225
#define WM_MDITILE             0x0226
#define WM_MDICASCADE          0x0227
#define WM_MDIICONARRANGE      0x0228
#define WM_MDIGETACTIVE        0x0229

/* D&D messages */
#define WM_DROPOBJECT          0x022A /* DDK / Win16 */
#define WM_QUERYDROPOBJECT     0x022B /* DDK / Win16 */
#define WM_BEGINDRAG           0x022C /* DDK / Win16 */
#define WM_DRAGLOOP            0x022D /* DDK / Win16 */
#define WM_DRAGSELECT          0x022E /* DDK / Win16 */
#define WM_DRAGMOVE            0x022F /* DDK / Win16 */

#define WM_MDISETMENU          0x0230
#define WM_ENTERSIZEMOVE       0x0231
#define WM_EXITSIZEMOVE        0x0232
#define WM_DROPFILES           0x0233
#define WM_MDIREFRESHMENU      0x0234

#define WM_POINTERDEVICECHANGE     0x0238
#define WM_POINTERDEVICEINRANGE    0x0239
#define WM_POINTERDEVICEOUTOFRANGE 0x023a

#define WM_TOUCH               0x0240
#define WM_NCPOINTERUPDATE     0x0241
#define WM_NCPOINTERDOWN       0x0242
#define WM_NCPOINTERUP         0x0243
#define WM_POINTERUPDATE       0x0245
#define WM_POINTERDOWN         0x0246
#define WM_POINTERUP           0x0247
#define WM_POINTERENTER        0x0249
#define WM_POINTERLEAVE        0x024a
#define WM_POINTERACTIVATE     0x024b
#define WM_POINTERCAPTURECHANGED 0x024c
#define WM_TOUCHHITTESTING     0x024d
#define WM_POINTERWHEEL        0x024e
#define WM_POINTERHWHEEL       0x024f
#define DM_POINTERHITTEST      0x0250
#define WM_POINTERROUTEDTO     0x0251
#define WM_POINTERROUTEDAWAY   0x0252
#define WM_POINTERROUTEDRELEASED 0x0253

/* Win32 4.0 messages for IME */
#define WM_IME_SETCONTEXT      0x0281
#define WM_IME_NOTIFY          0x0282
#define WM_IME_CONTROL         0x0283
#define WM_IME_COMPOSITIONFULL 0x0284
#define WM_IME_SELECT          0x0285
#define WM_IME_CHAR            0x0286
/* Win32 5.0 messages for IME */
#define WM_IME_REQUEST         0x0288

/* Win32 4.0 messages for IME */
#define WM_IME_KEYDOWN         0x0290
#define WM_IME_KEYUP           0x0291

#define WM_NCMOUSEHOVER        0x02A0
#define WM_MOUSEHOVER          0x02A1
#define WM_MOUSELEAVE          0x02A3
#define WM_NCMOUSELEAVE        0x02A2

#define WM_WTSSESSION_CHANGE   0x02B1

#define WM_TABLET_FIRST        0x02c0
#define WM_TABLET_LAST         0x02df

#define WM_DPICHANGED          0x02e0
#define WM_DPICHANGED_BEFOREPARENT 0x02e2
#define WM_DPICHANGED_AFTERPARENT  0x02e3
#define WM_GETDPISCALEDSIZE    0x02e4

/* Clipboard command messages */
#define WM_CUT                 0x0300
#define WM_COPY                0x0301
#define WM_PASTE               0x0302
#define WM_CLEAR               0x0303
#define WM_UNDO                0x0304

/* Clipboard owner messages */
#define WM_RENDERFORMAT        0x0305
#define WM_RENDERALLFORMATS    0x0306
#define WM_DESTROYCLIPBOARD    0x0307

/* Clipboard viewer messages */
#define WM_DRAWCLIPBOARD       0x0308
#define WM_PAINTCLIPBOARD      0x0309
#define WM_VSCROLLCLIPBOARD    0x030A
#define WM_SIZECLIPBOARD       0x030B
#define WM_ASKCBFORMATNAME     0x030C
#define WM_CHANGECBCHAIN       0x030D
#define WM_HSCROLLCLIPBOARD    0x030E

#define WM_QUERYNEWPALETTE     0x030F
#define WM_PALETTEISCHANGING   0x0310
#define WM_PALETTECHANGED      0x0311
#define WM_HOTKEY              0x0312

#define WM_PRINT               0x0317
#define WM_PRINTCLIENT         0x0318
#define WM_APPCOMMAND          0x0319
#define WM_THEMECHANGED        0x031A
#define WM_CLIPBOARDUPDATE     0x031D

#define WM_DWMCOMPOSITIONCHANGED 0x031E
#define WM_DWMNCRENDERINGCHANGED 0x031F
#define WM_DWMCOLORIZATIONCOLORCHANGED 0x0320
#define WM_DWMWINDOWMAXIMIZEDCHANGE 0x0321
#define WM_DWMSENDICONICTHUMBNAIL 0x0323
#define WM_DWMSENDICONICLIVEPREVIEWBITMAP 0x0326

#define WM_GETTITLEBARINFOEX   0x033F

#define WM_HANDHELDFIRST       0x0358
#define WM_HANDHELDLAST        0x035F

#define WM_AFXFIRST            0x0360
#define WM_AFXLAST             0x037F

#define WM_PENWINFIRST         0x0380
#define WM_PENWINLAST          0x038F

#define WM_USER                0x0400

#define WM_APP                 0x8000


/* wParam for WM_SIZING message */
#define WMSZ_LEFT              1
#define WMSZ_RIGHT             2
#define WMSZ_TOP               3
#define WMSZ_TOPLEFT           4
#define WMSZ_TOPRIGHT          5
#define WMSZ_BOTTOM            6
#define WMSZ_BOTTOMLEFT        7
#define WMSZ_BOTTOMRIGHT       8

/* WM_NCHITTEST return codes */
#define HTERROR                (-2)
#define HTTRANSPARENT          (-1)
#define HTNOWHERE              0
#define HTCLIENT               1
#define HTCAPTION              2
#define HTSYSMENU              3
#define HTSIZE                 4
#define HTGROWBOX              HTSIZE
#define HTMENU                 5
#define HTHSCROLL              6
#define HTVSCROLL              7
#define HTMINBUTTON            8
#define HTREDUCE               HTMINBUTTON
#define HTMAXBUTTON            9
#define HTZOOM                 HTMAXBUTTON
#define HTLEFT                 10
#define HTSIZEFIRST            HTLEFT
#define HTRIGHT                11
#define HTTOP                  12
#define HTTOPLEFT              13
#define HTTOPRIGHT             14
#define HTBOTTOM               15
#define HTBOTTOMLEFT           16
#define HTBOTTOMRIGHT          17
#define HTSIZELAST             HTBOTTOMRIGHT
#define HTBORDER               18
#define HTOBJECT               19
#define HTCLOSE                20
#define HTHELP                 21

/* SendMessageTimeout flags */
#define SMTO_NORMAL            0x0000
#define SMTO_BLOCK             0x0001
#define SMTO_ABORTIFHUNG       0x0002
#define SMTO_NOTIMEOUTIFNOTHUNG 0x0008
#define SMTO_ERRORONEXIT       0x0020

/* WM_MOUSEACTIVATE return values */
#define MA_ACTIVATE            1
#define MA_ACTIVATEANDEAT      2
#define MA_NOACTIVATE          3
#define MA_NOACTIVATEANDEAT    4

/* WM_GETICON/WM_SETICON params values */
#define ICON_SMALL             0
#define ICON_BIG               1
#define ICON_SMALL2            2

/* WM_SIZE message wParam values */
#define SIZE_RESTORED          0
#define SIZE_MINIMIZED         1
#define SIZE_MAXIMIZED         2
#define SIZE_MAXSHOW           3
#define SIZE_MAXHIDE           4
#define SIZENORMAL             SIZE_RESTORED
#define SIZEICONIC             SIZE_MINIMIZED
#define SIZEFULLSCREEN         SIZE_MAXIMIZED
#define SIZEZOOMSHOW           SIZE_MAXSHOW
#define SIZEZOOMHIDE           SIZE_MAXHIDE

/* WM_NCCALCSIZE return flags */
#define WVR_ALIGNTOP           0x0010
#define WVR_ALIGNLEFT          0x0020
#define WVR_ALIGNBOTTOM        0x0040
#define WVR_ALIGNRIGHT         0x0080
#define WVR_HREDRAW            0x0100
#define WVR_VREDRAW            0x0200
#define WVR_REDRAW             (WVR_HREDRAW | WVR_VREDRAW)
#define WVR_VALIDRECTS         0x0400

/* Key status flags for mouse events */
#ifndef NOKEYSTATES
#define MK_LBUTTON             0x0001
#define MK_RBUTTON             0x0002
#define MK_SHIFT               0x0004
#define MK_CONTROL             0x0008
#define MK_MBUTTON             0x0010
#define MK_XBUTTON1            0x0020
#define MK_XBUTTON2            0x0040
#endif /* NOKEYSTATES */

#ifndef NOTRACKMOUSEEVENT
#define TME_HOVER              0x00000001
#define TME_LEAVE              0x00000002
#define TME_NONCLIENT          0x00000010
#define TME_QUERY              0x40000000
#define TME_CANCEL             0x80000000
#define HOVER_DEFAULT          0xFFFFFFFF
#endif /* NOTRACKMOUSEEVENT */

#endif /* NOWINMESSAGES */


/*** Window Styles ***/
#ifndef NOWINSTYLES
#define WS_OVERLAPPED          __MSABI_LONG(0x00000000)
#define WS_POPUP               __MSABI_LONG(0x80000000)
#define WS_CHILD               __MSABI_LONG(0x40000000)
#define WS_MINIMIZE            __MSABI_LONG(0x20000000)
#define WS_VISIBLE             __MSABI_LONG(0x10000000)
#define WS_DISABLED            __MSABI_LONG(0x08000000)
#define WS_CLIPSIBLINGS        __MSABI_LONG(0x04000000)
#define WS_CLIPCHILDREN        __MSABI_LONG(0x02000000)
#define WS_MAXIMIZE            __MSABI_LONG(0x01000000)
#define WS_BORDER              __MSABI_LONG(0x00800000)
#define WS_DLGFRAME            __MSABI_LONG(0x00400000)
#define WS_VSCROLL             __MSABI_LONG(0x00200000)
#define WS_HSCROLL             __MSABI_LONG(0x00100000)
#define WS_SYSMENU             __MSABI_LONG(0x00080000)
#define WS_THICKFRAME          __MSABI_LONG(0x00040000)
#define WS_GROUP               __MSABI_LONG(0x00020000)
#define WS_TABSTOP             __MSABI_LONG(0x00010000)
#define WS_MINIMIZEBOX         __MSABI_LONG(0x00020000)
#define WS_MAXIMIZEBOX         __MSABI_LONG(0x00010000)
#define WS_CAPTION             (WS_BORDER | WS_DLGFRAME)
#define WS_TILED               WS_OVERLAPPED
#define WS_ICONIC              WS_MINIMIZE
#define WS_SIZEBOX             WS_THICKFRAME
#define WS_OVERLAPPEDWINDOW    (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME| WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW         (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_CHILDWINDOW         WS_CHILD
#define WS_TILEDWINDOW         WS_OVERLAPPEDWINDOW
#endif /* NOWINSTYLES */


/*** Window extended styles ***/
#ifndef NOWINSTYLES
#define WS_EX_DLGMODALFRAME    __MSABI_LONG(0x00000001)
#define WS_EX_DRAGDETECT       __MSABI_LONG(0x00000002) /* Undocumented */
#define WS_EX_NOPARENTNOTIFY   __MSABI_LONG(0x00000004)
#define WS_EX_TOPMOST          __MSABI_LONG(0x00000008)
#define WS_EX_ACCEPTFILES      __MSABI_LONG(0x00000010)
#define WS_EX_TRANSPARENT      __MSABI_LONG(0x00000020)
#define WS_EX_MDICHILD         __MSABI_LONG(0x00000040)
#define WS_EX_TOOLWINDOW       __MSABI_LONG(0x00000080)
#define WS_EX_WINDOWEDGE       __MSABI_LONG(0x00000100)
#define WS_EX_CLIENTEDGE       __MSABI_LONG(0x00000200)
#define WS_EX_CONTEXTHELP      __MSABI_LONG(0x00000400)
#define WS_EX_RIGHT            __MSABI_LONG(0x00001000)
#define WS_EX_LEFT             __MSABI_LONG(0x00000000)
#define WS_EX_RTLREADING       __MSABI_LONG(0x00002000)
#define WS_EX_LTRREADING       __MSABI_LONG(0x00000000)
#define WS_EX_LEFTSCROLLBAR    __MSABI_LONG(0x00004000)
#define WS_EX_RIGHTSCROLLBAR   __MSABI_LONG(0x00000000)
#define WS_EX_CONTROLPARENT    __MSABI_LONG(0x00010000)
#define WS_EX_STATICEDGE       __MSABI_LONG(0x00020000)
#define WS_EX_APPWINDOW        __MSABI_LONG(0x00040000)
#define WS_EX_LAYERED          __MSABI_LONG(0x00080000)
#define WS_EX_NOINHERITLAYOUT  __MSABI_LONG(0x00100000)
#define WS_EX_NOREDIRECTIONBITMAP __MSABI_LONG(0x00200000)
#define WS_EX_LAYOUTRTL        __MSABI_LONG(0x00400000)
#define WS_EX_COMPOSITED       __MSABI_LONG(0x02000000)
#define WS_EX_NOACTIVATE       __MSABI_LONG(0x08000000)

#define WS_EX_OVERLAPPEDWINDOW (WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE)
#define WS_EX_PALETTEWINDOW    (WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST)
#endif /* NOWINSTYLES */


/*** Class styles ***/
#ifndef NOWINSTYLES
#define CS_VREDRAW             0x00000001
#define CS_HREDRAW             0x00000002
#define CS_KEYCVTWINDOW        0x00000004 /* DDK / Win16 */
#define CS_DBLCLKS             0x00000008
#define CS_OWNDC               0x00000020
#define CS_CLASSDC             0x00000040
#define CS_PARENTDC            0x00000080
#define CS_NOKEYCVT            0x00000100 /* DDK / Win16 */
#define CS_NOCLOSE             0x00000200
#define CS_SAVEBITS            0x00000800
#define CS_BYTEALIGNCLIENT     0x00001000
#define CS_BYTEALIGNWINDOW     0x00002000
#define CS_GLOBALCLASS         0x00004000
#define CS_IME                 0x00010000
#define CS_DROPSHADOW          0x00020000
#endif /* NOWINSTYLES */


/*** Predefined Clipboard Formats ***/
#ifndef NOCLIPBOARD
#define CF_TEXT                1
#define CF_BITMAP              2
#define CF_METAFILEPICT        3
#define CF_SYLK                4
#define CF_DIF                 5
#define CF_TIFF                6
#define CF_OEMTEXT             7
#define CF_DIB                 8
#define CF_PALETTE             9
#define CF_PENDATA             10
#define CF_RIFF                11
#define CF_WAVE                12
#define CF_UNICODETEXT         13
#define CF_ENHMETAFILE         14
#define CF_HDROP               15
#define CF_LOCALE              16
#define CF_DIBV5               17
#define CF_MAX                 18

#define CF_OWNERDISPLAY        0x0080
#define CF_DSPTEXT             0x0081
#define CF_DSPBITMAP           0x0082
#define CF_DSPMETAFILEPICT     0x0083
#define CF_DSPENHMETAFILE      0x008E

/* "Private" formats don't get GlobalFree()'d */
#define CF_PRIVATEFIRST        0x0200
#define CF_PRIVATELAST         0x02FF

/* "GDIOBJ" formats do get DeleteObject()'d */
#define CF_GDIOBJFIRST         0x0300
#define CF_GDIOBJLAST          0x03FF
#endif /* NOCLIPBOARD */


/*** Menu flags ***/
#ifndef NOMENUS
#define MF_INSERT              __MSABI_LONG(0x00000000)
#define MF_CHANGE              __MSABI_LONG(0x00000080)
#define MF_APPEND              __MSABI_LONG(0x00000100)
#define MF_DELETE              __MSABI_LONG(0x00000200)
#define MF_REMOVE              __MSABI_LONG(0x00001000)
#define MF_END                 __MSABI_LONG(0x00000080)

#define MF_ENABLED             __MSABI_LONG(0x00000000)
#define MF_GRAYED              __MSABI_LONG(0x00000001)
#define MF_DISABLED            __MSABI_LONG(0x00000002)
#define MF_STRING              __MSABI_LONG(0x00000000)
#define MF_BITMAP              __MSABI_LONG(0x00000004)
#define MF_UNCHECKED           __MSABI_LONG(0x00000000)
#define MF_CHECKED             __MSABI_LONG(0x00000008)
#define MF_POPUP               __MSABI_LONG(0x00000010)
#define MF_MENUBARBREAK        __MSABI_LONG(0x00000020)
#define MF_MENUBREAK           __MSABI_LONG(0x00000040)
#define MF_UNHILITE            __MSABI_LONG(0x00000000)
#define MF_HILITE              __MSABI_LONG(0x00000080)
#define MF_OWNERDRAW           __MSABI_LONG(0x00000100)
#define MF_USECHECKBITMAPS     __MSABI_LONG(0x00000200)
#define MF_BYCOMMAND           __MSABI_LONG(0x00000000)
#define MF_BYPOSITION          __MSABI_LONG(0x00000400)
#define MF_SEPARATOR           __MSABI_LONG(0x00000800)
#define MF_DEFAULT             __MSABI_LONG(0x00001000)
#define MF_SYSMENU             __MSABI_LONG(0x00002000)
#define MF_HELP                __MSABI_LONG(0x00004000)
#define MF_RIGHTJUSTIFY        __MSABI_LONG(0x00004000)
#define MF_MOUSESELECT         __MSABI_LONG(0x00008000)

/* Flags for extended menu item types */
#define MFT_STRING             MF_STRING
#define MFT_BITMAP             MF_BITMAP
#define MFT_MENUBARBREAK       MF_MENUBARBREAK
#define MFT_MENUBREAK          MF_MENUBREAK
#define MFT_OWNERDRAW          MF_OWNERDRAW
#define MFT_RADIOCHECK         __MSABI_LONG(0x00000200)
#define MFT_SEPARATOR          MF_SEPARATOR
#define MFT_RIGHTORDER         __MSABI_LONG(0x00002000)
#define MFT_RIGHTJUSTIFY       MF_RIGHTJUSTIFY

/* Flags for extended menu item states */
#define MFS_GRAYED             __MSABI_LONG(0x00000003)
#define MFS_DISABLED           MFS_GRAYED
#define MFS_CHECKED            MF_CHECKED
#define MFS_HILITE             MF_HILITE
#define MFS_ENABLED            MF_ENABLED
#define MFS_UNCHECKED          MF_UNCHECKED
#define MFS_UNHILITE           MF_UNHILITE
#define MFS_DEFAULT            MF_DEFAULT

/* DDK / Win16 defines */
#define MFS_MASK               __MSABI_LONG(0x0000108B)
#define MFS_HOTTRACKDRAWN      __MSABI_LONG(0x10000000)
#define MFS_CACHEDBMP          __MSABI_LONG(0x20000000)
#define MFS_BOTTOMGAPDROP      __MSABI_LONG(0x40000000)
#define MFS_TOPGAPDROP         __MSABI_LONG(0x80000000)
#define MFS_GAPDROP            (MFS_BOTTOMGAPDROP | MFS_TOPGAPDROP)
#endif /* NOMENUS */


/*** WM_SYSCOMMAND parameters ***/
#ifndef NOSYSCOMMANDS
/* At least HP-UX defines it in /usr/include/sys/signal.h */
# ifdef SC_SIZE
#  undef SC_SIZE
# endif
#define SC_SIZE                0xf000
#define SC_MOVE                0xf010
#define SC_MINIMIZE            0xf020
#define SC_MAXIMIZE            0xf030
#define SC_NEXTWINDOW          0xf040
#define SC_PREVWINDOW          0xf050
#define SC_CLOSE               0xf060
#define SC_VSCROLL             0xf070
#define SC_HSCROLL             0xf080
#define SC_MOUSEMENU           0xf090
#define SC_KEYMENU             0xf100
#define SC_ARRANGE             0xf110
#define SC_RESTORE             0xf120
#define SC_TASKLIST            0xf130
#define SC_SCREENSAVE          0xf140
#define SC_HOTKEY              0xf150

/* Win32 4.0 */
#define SC_DEFAULT             0xf160
#define SC_MONITORPOWER        0xf170
#define SC_CONTEXTHELP         0xf180
#define SC_SEPARATOR           0xf00f

#define GET_SC_WPARAM(wParam)  ((int)wParam & 0xfff0)
#define SCF_ISSECURE           0x0001

/* Obsolete names */
#define SC_ICON               SC_MINIMIZE
#define SC_ZOOM               SC_MAXIMIZE
#endif /* NOSYSCOMMANDS */


/*** OEM Resource Ordinal Numbers ***/
#ifdef OEMRESOURCE
#define OBM_RDRVERT            32559
#define OBM_RDRHORZ            32660
#define OBM_RDR2DIM            32661
#define OBM_TRTYPE             32732 /* FIXME: Wine-only */
#define OBM_LFARROWI           32734
#define OBM_RGARROWI           32735
#define OBM_DNARROWI           32736
#define OBM_UPARROWI           32737
#define OBM_COMBO              32738
#define OBM_MNARROW            32739
#define OBM_LFARROWD           32740
#define OBM_RGARROWD           32741
#define OBM_DNARROWD           32742
#define OBM_UPARROWD           32743
#define OBM_RESTORED           32744
#define OBM_ZOOMD              32745
#define OBM_REDUCED            32746
#define OBM_RESTORE            32747
#define OBM_ZOOM               32748
#define OBM_REDUCE             32749
#define OBM_LFARROW            32750
#define OBM_RGARROW            32751
#define OBM_DNARROW            32752
#define OBM_UPARROW            32753
#define OBM_CLOSE              32754
#define OBM_OLD_RESTORE        32755
#define OBM_OLD_ZOOM           32756
#define OBM_OLD_REDUCE         32757
#define OBM_BTNCORNERS         32758
#define OBM_CHECKBOXES         32759
#define OBM_CHECK              32760
#define OBM_BTSIZE             32761
#define OBM_OLD_LFARROW        32762
#define OBM_OLD_RGARROW        32763
#define OBM_OLD_DNARROW        32764
#define OBM_OLD_UPARROW        32765
#define OBM_SIZE               32766
#define OBM_OLD_CLOSE          32767

#define OCR_NORMAL             32512
#define OCR_IBEAM              32513
#define OCR_WAIT               32514
#define OCR_CROSS              32515
#define OCR_UP                 32516
#define OCR_PEN                32631
#define OCR_SIZE               32640
#define OCR_ICON               32641
#define OCR_SIZENWSE           32642
#define OCR_SIZENESW           32643
#define OCR_SIZEWE             32644
#define OCR_SIZENS             32645
#define OCR_SIZEALL            32646
#define OCR_ICOCUR             32647
#define OCR_NO                 32648
#define OCR_HAND               32649
#define OCR_APPSTARTING        32650
#define OCR_HELP               32651 /* DDK / Win16 */
#define OCR_RDRVERT            32652 /* DDK / Win16 */
#define OCR_RDRHORZ            32653 /* DDK / Win16 */
#define OCR_RDR2DIM            32654 /* DDK / Win16 */
#define OCR_RDRNORTH           32655 /* DDK / Win16 */
#define OCR_RDRSOUTH           32656 /* DDK / Win16 */
#define OCR_RDRWEST            32657 /* DDK / Win16 */
#define OCR_RDREAST            32658 /* DDK / Win16 */
#define OCR_RDRNORTHWEST       32659 /* DDK / Win16 */
#define OCR_RDRNORTHEAST       32660 /* DDK / Win16 */
#define OCR_RDRSOUTHWEST       32661 /* DDK / Win16 */
#define OCR_RDRSOUTHEAST       32662 /* DDK / Win16 */

#define OIC_SAMPLE             32512
#define OIC_HAND               32513
#define OIC_ERROR              OIC_HAND
#define OIC_QUES               32514
#define OIC_BANG               32515
#define OIC_WARNING            OIC_BANG
#define OIC_NOTE               32516
#define OIC_INFORMATION        OIC_NOTE
#define OIC_WINLOGO            32517
#define OIC_SHIELD             32518
#endif /* OEMRESOURCE */


/*** Predefined resources ***/
#ifndef NOICONS
#define IDI_APPLICATION        MAKEINTRESOURCE(32512)
#define IDI_HAND               MAKEINTRESOURCE(32513)
#define IDI_QUESTION           MAKEINTRESOURCE(32514)
#define IDI_EXCLAMATION        MAKEINTRESOURCE(32515)
#define IDI_ASTERISK           MAKEINTRESOURCE(32516)
#define IDI_WINLOGO            MAKEINTRESOURCE(32517)
#define IDI_SHIELD             MAKEINTRESOURCE(32518)

#define IDI_WARNING            IDI_EXCLAMATION
#define IDI_ERROR              IDI_HAND
#define IDI_INFORMATION        IDI_ASTERISK
#endif /* NOICONS */


/*** Standard dialog button IDs ***/
#define IDOK                   1
#define IDCANCEL               2
#define IDABORT                3
#define IDRETRY                4
#define IDIGNORE               5
#define IDYES                  6
#define IDNO                   7
#define IDCLOSE                8
#define IDHELP                 9
#define IDTRYAGAIN             10
#define IDCONTINUE             11
#ifndef IDTIMEOUT
#define IDTIMEOUT              32000
#endif


/*** Edit control styles ***/
#ifndef NOWINSTYLES
#define ES_LEFT                __MSABI_LONG(0x00000000)
#define ES_CENTER              __MSABI_LONG(0x00000001)
#define ES_RIGHT               __MSABI_LONG(0x00000002)
#define ES_MULTILINE           __MSABI_LONG(0x00000004)
#define ES_UPPERCASE           __MSABI_LONG(0x00000008)
#define ES_LOWERCASE           __MSABI_LONG(0x00000010)
#define ES_PASSWORD            __MSABI_LONG(0x00000020)
#define ES_AUTOVSCROLL         __MSABI_LONG(0x00000040)
#define ES_AUTOHSCROLL         __MSABI_LONG(0x00000080)
#define ES_NOHIDESEL           __MSABI_LONG(0x00000100)
#define ES_COMBO               __MSABI_LONG(0x00000200) /* Undocumented. Parent is a combobox */
#define ES_OEMCONVERT          __MSABI_LONG(0x00000400)
#define ES_READONLY            __MSABI_LONG(0x00000800)
#define ES_WANTRETURN          __MSABI_LONG(0x00001000)
#define ES_NUMBER              __MSABI_LONG(0x00002000)
#endif /* NOWINSTYLES */


/*** Edit control messages ***/
#ifndef NOWINMESSAGES
#define EM_GETSEL              0x00b0
#define EM_SETSEL              0x00b1
#define EM_GETRECT             0x00b2
#define EM_SETRECT             0x00b3
#define EM_SETRECTNP           0x00b4
#define EM_SCROLL              0x00b5
#define EM_LINESCROLL          0x00b6
#define EM_SCROLLCARET         0x00b7
#define EM_GETMODIFY           0x00b8
#define EM_SETMODIFY           0x00b9
#define EM_GETLINECOUNT        0x00ba
#define EM_LINEINDEX           0x00bb
#define EM_SETHANDLE           0x00bc
#define EM_GETHANDLE           0x00bd
#define EM_GETTHUMB            0x00be
/* Unassigned 0x00bf and 0x00c0 */
#define EM_LINELENGTH          0x00c1
#define EM_REPLACESEL          0x00c2
#define EM_SETFONT             0x00c3 /* DDK / Win16 */
#define EM_GETLINE             0x00c4
#define EM_LIMITTEXT           0x00c5
#define EM_SETLIMITTEXT        EM_LIMITTEXT
#define EM_CANUNDO             0x00c6
#define EM_UNDO                0x00c7
#define EM_FMTLINES            0x00c8
#define EM_LINEFROMCHAR        0x00c9
#define EM_SETWORDBREAK        0x00ca /* DDK / Win16 */
#define EM_SETTABSTOPS         0x00cb
#define EM_SETPASSWORDCHAR     0x00cc
#define EM_EMPTYUNDOBUFFER     0x00cd
#define EM_GETFIRSTVISIBLELINE 0x00ce
#define EM_SETREADONLY         0x00cf
#define EM_SETWORDBREAKPROC    0x00d0
#define EM_GETWORDBREAKPROC    0x00d1
#define EM_GETPASSWORDCHAR     0x00d2
#define EM_SETMARGINS          0x00d3
#define EM_GETMARGINS          0x00d4
#define EM_GETLIMITTEXT        0x00d5
#define EM_POSFROMCHAR         0x00d6
#define EM_CHARFROMPOS         0x00d7
#define EM_SETIMESTATUS        0x00d8
#define EM_GETIMESTATUS        0x00d9
#endif /* NOWINMESSAGES */


/*** Button control styles ***/
#define BS_PUSHBUTTON          __MSABI_LONG(0x00000000)
#define BS_DEFPUSHBUTTON       __MSABI_LONG(0x00000001)
#define BS_CHECKBOX            __MSABI_LONG(0x00000002)
#define BS_AUTOCHECKBOX        __MSABI_LONG(0x00000003)
#define BS_RADIOBUTTON         __MSABI_LONG(0x00000004)
#define BS_3STATE              __MSABI_LONG(0x00000005)
#define BS_AUTO3STATE          __MSABI_LONG(0x00000006)
#define BS_GROUPBOX            __MSABI_LONG(0x00000007)
#define BS_USERBUTTON          __MSABI_LONG(0x00000008)
#define BS_AUTORADIOBUTTON     __MSABI_LONG(0x00000009)
#define BS_PUSHBOX             __MSABI_LONG(0x0000000A)
#define BS_OWNERDRAW           __MSABI_LONG(0x0000000B)
#define BS_TYPEMASK            __MSABI_LONG(0x0000000F)
#define BS_LEFTTEXT            __MSABI_LONG(0x00000020)
#define BS_RIGHTBUTTON         BS_LEFTTEXT

#define BS_TEXT                __MSABI_LONG(0x00000000)
#define BS_ICON                __MSABI_LONG(0x00000040)
#define BS_BITMAP              __MSABI_LONG(0x00000080)
#define BS_LEFT                __MSABI_LONG(0x00000100)
#define BS_RIGHT               __MSABI_LONG(0x00000200)
#define BS_CENTER              __MSABI_LONG(0x00000300)
#define BS_TOP                 __MSABI_LONG(0x00000400)
#define BS_BOTTOM              __MSABI_LONG(0x00000800)
#define BS_VCENTER             __MSABI_LONG(0x00000C00)
#define BS_PUSHLIKE            __MSABI_LONG(0x00001000)
#define BS_MULTILINE           __MSABI_LONG(0x00002000)
#define BS_NOTIFY              __MSABI_LONG(0x00004000)
#define BS_FLAT                __MSABI_LONG(0x00008000)


/*** Button notification codes ***/
#define BN_CLICKED             0
#define BN_PAINT               1
#define BN_HILITE              2
#define BN_PUSHED              BN_HILITE
#define BN_UNHILITE            3
#define BN_UNPUSHED            BN_UNHILITE
#define BN_DISABLE             4
#define BN_DOUBLECLICKED       5
#define BN_DBLCLK              BN_DOUBLECLICKED
#define BN_SETFOCUS            6
#define BN_KILLFOCUS           7


/*** Win32 button control messages ***/
#define BM_GETCHECK            0x00f0
#define BM_SETCHECK            0x00f1
#define BM_GETSTATE            0x00f2
#define BM_SETSTATE            0x00f3
#define BM_SETSTYLE            0x00f4
#define BM_CLICK               0x00f5
#define BM_GETIMAGE            0x00f6
#define BM_SETIMAGE            0x00f7
#define BM_SETDONTCLICK        0x00f8

/* Button states */
#define BST_UNCHECKED          0x0000
#define BST_CHECKED            0x0001
#define BST_INDETERMINATE      0x0002
#define BST_PUSHED             0x0004
#define BST_FOCUS              0x0008

/*** Static Control Styles ***/
#define SS_LEFT                __MSABI_LONG(0x00000000)
#define SS_CENTER              __MSABI_LONG(0x00000001)
#define SS_RIGHT               __MSABI_LONG(0x00000002)
#define SS_ICON                __MSABI_LONG(0x00000003)
#define SS_BLACKRECT           __MSABI_LONG(0x00000004)
#define SS_GRAYRECT            __MSABI_LONG(0x00000005)
#define SS_WHITERECT           __MSABI_LONG(0x00000006)
#define SS_BLACKFRAME          __MSABI_LONG(0x00000007)
#define SS_GRAYFRAME           __MSABI_LONG(0x00000008)
#define SS_WHITEFRAME          __MSABI_LONG(0x00000009)
#define SS_USERITEM            __MSABI_LONG(0x0000000A)
#define SS_SIMPLE              __MSABI_LONG(0x0000000B)
#define SS_LEFTNOWORDWRAP      __MSABI_LONG(0x0000000C)
#define SS_OWNERDRAW           __MSABI_LONG(0x0000000D)
#define SS_BITMAP              __MSABI_LONG(0x0000000E)
#define SS_ENHMETAFILE         __MSABI_LONG(0x0000000F)
#define SS_ETCHEDHORZ          __MSABI_LONG(0x00000010)
#define SS_ETCHEDVERT          __MSABI_LONG(0x00000011)
#define SS_ETCHEDFRAME         __MSABI_LONG(0x00000012)
#define SS_TYPEMASK            __MSABI_LONG(0x0000001F)

#define SS_REALSIZECONTROL     __MSABI_LONG(0x00000040)
#define SS_NOPREFIX            __MSABI_LONG(0x00000080)
#define SS_NOTIFY              __MSABI_LONG(0x00000100)
#define SS_CENTERIMAGE         __MSABI_LONG(0x00000200)
#define SS_RIGHTJUST           __MSABI_LONG(0x00000400)
#define SS_REALSIZEIMAGE       __MSABI_LONG(0x00000800)
#define SS_SUNKEN              __MSABI_LONG(0x00001000)
#define SS_EDITCONTROL         __MSABI_LONG(0x00002000)
#define SS_ENDELLIPSIS         __MSABI_LONG(0x00004000)
#define SS_PATHELLIPSIS        __MSABI_LONG(0x00008000)
#define SS_WORDELLIPSIS        __MSABI_LONG(0x0000C000)
#define SS_ELLIPSISMASK        SS_WORDELLIPSIS


/*** Dialog styles ***/
#define DS_ABSALIGN            __MSABI_LONG(0x00000001)
#define DS_SYSMODAL            __MSABI_LONG(0x00000002)
#define DS_3DLOOK              __MSABI_LONG(0x00000004) /* win95 */
#define DS_FIXEDSYS            __MSABI_LONG(0x00000008) /* win95 */
#define DS_NOFAILCREATE        __MSABI_LONG(0x00000010) /* win95 */
#define DS_LOCALEDIT           __MSABI_LONG(0x00000020)
#define DS_SETFONT             __MSABI_LONG(0x00000040)
#define DS_MODALFRAME          __MSABI_LONG(0x00000080)
#define DS_NOIDLEMSG           __MSABI_LONG(0x00000100)
#define DS_SETFOREGROUND       __MSABI_LONG(0x00000200) /* win95 */
#define DS_CONTROL             __MSABI_LONG(0x00000400) /* win95 */
#define DS_CENTER              __MSABI_LONG(0x00000800) /* win95 */
#define DS_CENTERMOUSE         __MSABI_LONG(0x00001000) /* win95 */
#define DS_CONTEXTHELP         __MSABI_LONG(0x00002000) /* win95 */
#define DS_USEPIXELS           __MSABI_LONG(0x00008000)
#define DS_SHELLFONT           (DS_SETFONT | DS_FIXEDSYS)


/*** Listbox styles ***/
#ifndef NOWINSTYLES
#define LBS_NOTIFY             __MSABI_LONG(0x00000001)
#define LBS_SORT               __MSABI_LONG(0x00000002)
#define LBS_NOREDRAW           __MSABI_LONG(0x00000004)
#define LBS_MULTIPLESEL        __MSABI_LONG(0x00000008)
#define LBS_OWNERDRAWFIXED     __MSABI_LONG(0x00000010)
#define LBS_OWNERDRAWVARIABLE  __MSABI_LONG(0x00000020)
#define LBS_HASSTRINGS         __MSABI_LONG(0x00000040)
#define LBS_USETABSTOPS        __MSABI_LONG(0x00000080)
#define LBS_NOINTEGRALHEIGHT   __MSABI_LONG(0x00000100)
#define LBS_MULTICOLUMN        __MSABI_LONG(0x00000200)
#define LBS_WANTKEYBOARDINPUT  __MSABI_LONG(0x00000400)
#define LBS_EXTENDEDSEL        __MSABI_LONG(0x00000800)
#define LBS_DISABLENOSCROLL    __MSABI_LONG(0x00001000)
#define LBS_NODATA             __MSABI_LONG(0x00002000)
#define LBS_NOSEL              __MSABI_LONG(0x00004000)
#define LBS_COMBOBOX           __MSABI_LONG(0x00008000)
#define LBS_STANDARD           (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)
#endif /* NOWINSTYLES */

/*** Combo box styles ***/
#ifndef NOWINSTYLES
#define CBS_SIMPLE             __MSABI_LONG(0x00000001)
#define CBS_DROPDOWN           __MSABI_LONG(0x00000002)
#define CBS_DROPDOWNLIST       __MSABI_LONG(0x00000003)
#define CBS_OWNERDRAWFIXED     __MSABI_LONG(0x00000010)
#define CBS_OWNERDRAWVARIABLE  __MSABI_LONG(0x00000020)
#define CBS_AUTOHSCROLL        __MSABI_LONG(0x00000040)
#define CBS_OEMCONVERT         __MSABI_LONG(0x00000080)
#define CBS_SORT               __MSABI_LONG(0x00000100)
#define CBS_HASSTRINGS         __MSABI_LONG(0x00000200)
#define CBS_NOINTEGRALHEIGHT   __MSABI_LONG(0x00000400)
#define CBS_DISABLENOSCROLL    __MSABI_LONG(0x00000800)

#define CBS_UPPERCASE          __MSABI_LONG(0x00002000)
#define CBS_LOWERCASE          __MSABI_LONG(0x00004000)
#endif /* NOWINSTYLES */


/*** Scrollbar styles ***/
#ifndef NOWINSTYLES
#define SBS_HORZ               __MSABI_LONG(0x00000000)
#define SBS_VERT               __MSABI_LONG(0x00000001)
#define SBS_TOPALIGN           __MSABI_LONG(0x00000002)
#define SBS_LEFTALIGN          __MSABI_LONG(0x00000002)
#define SBS_BOTTOMALIGN        __MSABI_LONG(0x00000004)
#define SBS_RIGHTALIGN         __MSABI_LONG(0x00000004)
#define SBS_SIZEBOXTOPLEFTALIGN __MSABI_LONG(0x00000002)
#define SBS_SIZEBOXBOTTOMRIGHTALIGN __MSABI_LONG(0x00000004)
#define SBS_SIZEBOX            __MSABI_LONG(0x00000008)
#define SBS_SIZEGRIP           __MSABI_LONG(0x00000010)
#endif /* NOWINSTYLES */

/*** WinHelp commands ***/
#define HELP_CONTEXT           __MSABI_LONG(0x00000001)
#define HELP_QUIT              __MSABI_LONG(0x00000002)
#define HELP_INDEX             __MSABI_LONG(0x00000003)
#define HELP_CONTENTS          HELP_INDEX
#define HELP_HELPONHELP        __MSABI_LONG(0x00000004)
#define HELP_SETINDEX          __MSABI_LONG(0x00000005)
#define HELP_SETCONTENTS       HELP_SETINDEX
#define HELP_CONTEXTPOPUP      __MSABI_LONG(0x00000008)
#define HELP_FORCEFILE         __MSABI_LONG(0x00000009)
#define HELP_KEY               __MSABI_LONG(0x00000101)
#define HELP_COMMAND           __MSABI_LONG(0x00000102)
#define HELP_PARTIALKEY        __MSABI_LONG(0x00000105)
#define HELP_MULTIKEY          __MSABI_LONG(0x00000201)
#define HELP_SETWINPOS         __MSABI_LONG(0x00000203)

#define HELP_CONTEXTMENU       0x000a
#define HELP_FINDER            0x000b
#define HELP_WM_HELP           0x000c
#define HELP_SETPOPUP_POS      0x000d
#define HELP_TCARD_DATA        0x0010
#define HELP_TCARD_OTHER_CALLER 0x0011
#define HELP_TCARD             0x8000

#define IDH_NO_HELP            28440
#define IDH_MISSING_CONTEXT    28441
#define IDH_GENERIC_HELP_BUTTON 28442
#define IDH_OK                 28443
#define IDH_CANCEL             28444
#define IDH_HELP               28445
