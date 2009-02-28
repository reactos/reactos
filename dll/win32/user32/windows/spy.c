/*
 * Message spying routines
 *
 * Copyright 1994, Bob Amstadt
 *           1995, Alex Korobka
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

#include <user32.h>
#include <commctrl.h>
#include <richedit.h>
#include <prsht.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(message);

#define SPY_MAX_MSGNUM   WM_USER
#define SPY_INDENT_UNIT  4  /* 4 spaces */

#define DEBUG_SPY 0

static const char * const MessageTypeNames[SPY_MAX_MSGNUM + 1] =
{
    "WM_NULL",                  /* 0x00 */
    "WM_CREATE",
    "WM_DESTROY",
    "WM_MOVE",
    "wm_sizewait",
    "WM_SIZE",
    "WM_ACTIVATE",
    "WM_SETFOCUS",
    "WM_KILLFOCUS",
    "WM_SETVISIBLE",
    "WM_ENABLE",
    "WM_SETREDRAW",
    "WM_SETTEXT",
    "WM_GETTEXT",
    "WM_GETTEXTLENGTH",
    "WM_PAINT",
    "WM_CLOSE",                 /* 0x10 */
    "WM_QUERYENDSESSION",
    "WM_QUIT",
    "WM_QUERYOPEN",
    "WM_ERASEBKGND",
    "WM_SYSCOLORCHANGE",
    "WM_ENDSESSION",
    "wm_systemerror",
    "WM_SHOWWINDOW",
    "WM_CTLCOLOR",
    "WM_WININICHANGE",
    "WM_DEVMODECHANGE",
    "WM_ACTIVATEAPP",
    "WM_FONTCHANGE",
    "WM_TIMECHANGE",
    "WM_CANCELMODE",
    "WM_SETCURSOR",             /* 0x20 */
    "WM_MOUSEACTIVATE",
    "WM_CHILDACTIVATE",
    "WM_QUEUESYNC",
    "WM_GETMINMAXINFO",
    "wm_unused3",
    "wm_painticon",
    "WM_ICONERASEBKGND",
    "WM_NEXTDLGCTL",
    "wm_alttabactive",
    "WM_SPOOLERSTATUS",
    "WM_DRAWITEM",
    "WM_MEASUREITEM",
    "WM_DELETEITEM",
    "WM_VKEYTOITEM",
    "WM_CHARTOITEM",
    "WM_SETFONT",               /* 0x30 */
    "WM_GETFONT",
    "WM_SETHOTKEY",
    "WM_GETHOTKEY",
    "wm_filesyschange",
    "wm_isactiveicon",
    "wm_queryparkicon",
    "WM_QUERYDRAGICON",
    "wm_querysavestate",
    "WM_COMPAREITEM",
    "wm_testing",
    NULL,
    NULL,
    "WM_GETOBJECT",             /* 0x3d */
    "wm_activateshellwindow",
    NULL,

    NULL,                       /* 0x40 */
    "wm_compacting", NULL, NULL,
    "WM_COMMNOTIFY", NULL,
    "WM_WINDOWPOSCHANGING",     /* 0x0046 */
    "WM_WINDOWPOSCHANGED",      /* 0x0047 */
    "WM_POWER", NULL,
    "WM_COPYDATA",
    "WM_CANCELJOURNAL", NULL, NULL,
    "WM_NOTIFY", NULL,

    /* 0x0050 */
    "WM_INPUTLANGCHANGEREQUEST",
    "WM_INPUTLANGCHANGE",
    "WM_TCARD",
    "WM_HELP",
    "WM_USERCHANGED",
    "WM_NOTIFYFORMAT", NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0060 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0070 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL,
    "WM_CONTEXTMENU",
    "WM_STYLECHANGING",
    "WM_STYLECHANGED",
    "WM_DISPLAYCHANGE",
    "WM_GETICON",

    "WM_SETICON",               /* 0x0080 */
    "WM_NCCREATE",              /* 0x0081 */
    "WM_NCDESTROY",             /* 0x0082 */
    "WM_NCCALCSIZE",            /* 0x0083 */
    "WM_NCHITTEST",             /* 0x0084 */
    "WM_NCPAINT",               /* 0x0085 */
    "WM_NCACTIVATE",            /* 0x0086 */
    "WM_GETDLGCODE",            /* 0x0087 */
    "WM_SYNCPAINT",
    "WM_SYNCTASK", NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0090 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00A0 */
    "WM_NCMOUSEMOVE",           /* 0x00a0 */
    "WM_NCLBUTTONDOWN",         /* 0x00a1 */
    "WM_NCLBUTTONUP",           /* 0x00a2 */
    "WM_NCLBUTTONDBLCLK",       /* 0x00a3 */
    "WM_NCRBUTTONDOWN",         /* 0x00a4 */
    "WM_NCRBUTTONUP",           /* 0x00a5 */
    "WM_NCRBUTTONDBLCLK",       /* 0x00a6 */
    "WM_NCMBUTTONDOWN",         /* 0x00a7 */
    "WM_NCMBUTTONUP",           /* 0x00a8 */
    "WM_NCMBUTTONDBLCLK",       /* 0x00a9 */
    NULL,                       /* 0x00aa */
    "WM_NCXBUTTONDOWN",         /* 0x00ab */
    "WM_NCXBUTTONUP",           /* 0x00ac */
    "WM_NCXBUTTONDBLCLK",       /* 0x00ad */
    NULL,                       /* 0x00ae */
    NULL,                       /* 0x00af */

    /* 0x00B0 - Win32 Edit controls */
    "EM_GETSEL",                /* 0x00b0 */
    "EM_SETSEL",                /* 0x00b1 */
    "EM_GETRECT",               /* 0x00b2 */
    "EM_SETRECT",               /* 0x00b3 */
    "EM_SETRECTNP",             /* 0x00b4 */
    "EM_SCROLL",                /* 0x00b5 */
    "EM_LINESCROLL",            /* 0x00b6 */
    "EM_SCROLLCARET",           /* 0x00b7 */
    "EM_GETMODIFY",             /* 0x00b8 */
    "EM_SETMODIFY",             /* 0x00b9 */
    "EM_GETLINECOUNT",          /* 0x00ba */
    "EM_LINEINDEX",             /* 0x00bb */
    "EM_SETHANDLE",             /* 0x00bc */
    "EM_GETHANDLE",             /* 0x00bd */
    "EM_GETTHUMB",              /* 0x00be */
    NULL,                       /* 0x00bf */

    NULL,                       /* 0x00c0 */
    "EM_LINELENGTH",            /* 0x00c1 */
    "EM_REPLACESEL",            /* 0x00c2 */
    NULL,                       /* 0x00c3 */
    "EM_GETLINE",               /* 0x00c4 */
    "EM_LIMITTEXT",             /* 0x00c5 */
    "EM_CANUNDO",               /* 0x00c6 */
    "EM_UNDO",                  /* 0x00c7 */
    "EM_FMTLINES",              /* 0x00c8 */
    "EM_LINEFROMCHAR",          /* 0x00c9 */
    NULL,                       /* 0x00ca */
    "EM_SETTABSTOPS",           /* 0x00cb */
    "EM_SETPASSWORDCHAR",       /* 0x00cc */
    "EM_EMPTYUNDOBUFFER",       /* 0x00cd */
    "EM_GETFIRSTVISIBLELINE",   /* 0x00ce */
    "EM_SETREADONLY",           /* 0x00cf */

    "EM_SETWORDBREAKPROC",      /* 0x00d0 */
    "EM_GETWORDBREAKPROC",      /* 0x00d1 */
    "EM_GETPASSWORDCHAR",       /* 0x00d2 */
    "EM_SETMARGINS",            /* 0x00d3 */
    "EM_GETMARGINS",            /* 0x00d4 */
    "EM_GETLIMITTEXT",          /* 0x00d5 */
    "EM_POSFROMCHAR",           /* 0x00d6 */
    "EM_CHARFROMPOS",           /* 0x00d7 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00E0 - Win32 Scrollbars */
    "SBM_SETPOS",               /* 0x00e0 */
    "SBM_GETPOS",               /* 0x00e1 */
    "SBM_SETRANGE",             /* 0x00e2 */
    "SBM_GETRANGE",             /* 0x00e3 */
    "SBM_ENABLE_ARROWS",        /* 0x00e4 */
    NULL,
    "SBM_SETRANGEREDRAW",       /* 0x00e6 */
    NULL, NULL,
    "SBM_SETSCROLLINFO",        /* 0x00e9 */
    "SBM_GETSCROLLINFO",        /* 0x00ea */
    NULL, NULL, NULL, NULL, NULL,

    /* 0x00F0 - Win32 Buttons */
    "BM_GETCHECK",              /* 0x00f0 */
    "BM_SETCHECK",              /* 0x00f1 */
    "BM_GETSTATE",              /* 0x00f2 */
    "BM_SETSTATE",              /* 0x00f3 */
    "BM_SETSTYLE",              /* 0x00f4 */
    "BM_CLICK",                 /* 0x00f5 */
    "BM_GETIMAGE",              /* 0x00f6 */
    "BM_SETIMAGE",              /* 0x00f7 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_KEYDOWN",               /* 0x0100 */
    "WM_KEYUP",                 /* 0x0101 */
    "WM_CHAR",                  /* 0x0102 */
    "WM_DEADCHAR",              /* 0x0103 */
    "WM_SYSKEYDOWN",            /* 0x0104 */
    "WM_SYSKEYUP",              /* 0x0105 */
    "WM_SYSCHAR",               /* 0x0106 */
    "WM_SYSDEADCHAR",           /* 0x0107 */
    "WM_KEYLAST",               /* 0x0108 */
    NULL,
    "WM_CONVERTREQUEST",
    "WM_CONVERTRESULT",
    "WM_INTERIM",
    "WM_IME_STARTCOMPOSITION",  /* 0x010d */
    "WM_IME_ENDCOMPOSITION",    /* 0x010e */
    "WM_IME_COMPOSITION",       /* 0x010f */

    "WM_INITDIALOG",            /* 0x0110 */
    "WM_COMMAND",               /* 0x0111 */
    "WM_SYSCOMMAND",            /* 0x0112 */
    "WM_TIMER",                 /* 0x0113 */
    "WM_HSCROLL",               /* 0x0114 */
    "WM_VSCROLL",               /* 0x0115 */
    "WM_INITMENU",              /* 0x0116 */
    "WM_INITMENUPOPUP",         /* 0x0117 */
    "WM_SYSTIMER",              /* 0x0118 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_MENUSELECT",            /* 0x011f */

    "WM_MENUCHAR",              /* 0x0120 */
    "WM_ENTERIDLE",             /* 0x0121 */

    "WM_MENURBUTTONUP",         /* 0x0122 */
    "WM_MENUDRAG",              /* 0x0123 */
    "WM_MENUGETOBJECT",         /* 0x0124 */
    "WM_UNINITMENUPOPUP",       /* 0x0125 */
    "WM_MENUCOMMAND",           /* 0x0126 */
    "WM_CHANGEUISTATE",         /* 0x0127 */
    "WM_UPDATEUISTATE",         /* 0x0128 */
    "WM_QUERYUISTATE",          /* 0x0129 */

    NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0130 */
    NULL,
    "WM_LBTRACKPOINT",          /* 0x0131 */
    "WM_CTLCOLORMSGBOX",        /* 0x0132 */
    "WM_CTLCOLOREDIT",          /* 0x0133 */
    "WM_CTLCOLORLISTBOX",       /* 0x0134 */
    "WM_CTLCOLORBTN",           /* 0x0135 */
    "WM_CTLCOLORDLG",           /* 0x0136 */
    "WM_CTLCOLORSCROLLBAR",     /* 0x0137 */
    "WM_CTLCOLORSTATIC",        /* 0x0138 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0140 - Win32 Comboboxes */
    "CB_GETEDITSEL",            /* 0x0140 */
    "CB_LIMITTEXT",             /* 0x0141 */
    "CB_SETEDITSEL",            /* 0x0142 */
    "CB_ADDSTRING",             /* 0x0143 */
    "CB_DELETESTRING",          /* 0x0144 */
    "CB_DIR",                   /* 0x0145 */
    "CB_GETCOUNT",              /* 0x0146 */
    "CB_GETCURSEL",             /* 0x0147 */
    "CB_GETLBTEXT",             /* 0x0148 */
    "CB_GETLBTEXTLEN",          /* 0x0149 */
    "CB_INSERTSTRING",          /* 0x014a */
    "CB_RESETCONTENT",          /* 0x014b */
    "CB_FINDSTRING",            /* 0x014c */
    "CB_SELECTSTRING",          /* 0x014d */
    "CB_SETCURSEL",             /* 0x014e */
    "CB_SHOWDROPDOWN",          /* 0x014f */

    "CB_GETITEMDATA",           /* 0x0150 */
    "CB_SETITEMDATA",           /* 0x0151 */
    "CB_GETDROPPEDCONTROLRECT", /* 0x0152 */
    "CB_SETITEMHEIGHT",         /* 0x0153 */
    "CB_GETITEMHEIGHT",         /* 0x0154 */
    "CB_SETEXTENDEDUI",         /* 0x0155 */
    "CB_GETEXTENDEDUI",         /* 0x0156 */
    "CB_GETDROPPEDSTATE",       /* 0x0157 */
    "CB_FINDSTRINGEXACT",       /* 0x0158 */
    "CB_SETLOCALE",             /* 0x0159 */
    "CB_GETLOCALE",             /* 0x015a */
    "CB_GETTOPINDEX",           /* 0x015b */
    "CB_SETTOPINDEX",           /* 0x015c */
    "CB_GETHORIZONTALEXTENT",   /* 0x015d */
    "CB_SETHORIZONTALEXTENT",   /* 0x015e */
    "CB_GETDROPPEDWIDTH",       /* 0x015f */

    "CB_SETDROPPEDWIDTH",       /* 0x0160 */
    "CB_INITSTORAGE",           /* 0x0161 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0170 - Win32 Static controls */
    "STM_SETICON",              /* 0x0170 */
    "STM_GETICON",              /* 0x0171 */
    "STM_SETIMAGE",             /* 0x0172 */
    "STM_GETIMAGE",             /* 0x0173 */
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0180 - Win32 Listboxes */
    "LB_ADDSTRING",             /* 0x0180 */
    "LB_INSERTSTRING",          /* 0x0181 */
    "LB_DELETESTRING",          /* 0x0182 */
    "LB_SELITEMRANGEEX",        /* 0x0183 */
    "LB_RESETCONTENT",          /* 0x0184 */
    "LB_SETSEL",                /* 0x0185 */
    "LB_SETCURSEL",             /* 0x0186 */
    "LB_GETSEL",                /* 0x0187 */
    "LB_GETCURSEL",             /* 0x0188 */
    "LB_GETTEXT",               /* 0x0189 */
    "LB_GETTEXTLEN",            /* 0x018a */
    "LB_GETCOUNT",              /* 0x018b */
    "LB_SELECTSTRING",          /* 0x018c */
    "LB_DIR",                   /* 0x018d */
    "LB_GETTOPINDEX",           /* 0x018e */
    "LB_FINDSTRING",            /* 0x018f */

    "LB_GETSELCOUNT",           /* 0x0190 */
    "LB_GETSELITEMS",           /* 0x0191 */
    "LB_SETTABSTOPS",           /* 0x0192 */
    "LB_GETHORIZONTALEXTENT",   /* 0x0193 */
    "LB_SETHORIZONTALEXTENT",   /* 0x0194 */
    "LB_SETCOLUMNWIDTH",        /* 0x0195 */
    "LB_ADDFILE",               /* 0x0196 */
    "LB_SETTOPINDEX",           /* 0x0197 */
    "LB_GETITEMRECT",           /* 0x0198 */
    "LB_GETITEMDATA",           /* 0x0199 */
    "LB_SETITEMDATA",           /* 0x019a */
    "LB_SELITEMRANGE",          /* 0x019b */
    "LB_SETANCHORINDEX",        /* 0x019c */
    "LB_GETANCHORINDEX",        /* 0x019d */
    "LB_SETCARETINDEX",         /* 0x019e */
    "LB_GETCARETINDEX",         /* 0x019f */

    "LB_SETITEMHEIGHT",         /* 0x01a0 */
    "LB_GETITEMHEIGHT",         /* 0x01a1 */
    "LB_FINDSTRINGEXACT",       /* 0x01a2 */
    "LB_CARETON",               /* 0x01a3 */
    "LB_CARETOFF",              /* 0x01a4 */
    "LB_SETLOCALE",             /* 0x01a5 */
    "LB_GETLOCALE",             /* 0x01a6 */
    "LB_SETCOUNT",              /* 0x01a7 */
    "LB_INITSTORAGE",           /* 0x01a8 */
    "LB_ITEMFROMPOINT",         /* 0x01a9 */
    NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01B0 */
    NULL, NULL,
    "LB_GETLISTBOXINFO",         /* 0x01b2 */
    NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01C0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01D0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01E0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01F0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_MOUSEMOVE",             /* 0x0200 */
    "WM_LBUTTONDOWN",           /* 0x0201 */
    "WM_LBUTTONUP",             /* 0x0202 */
    "WM_LBUTTONDBLCLK",         /* 0x0203 */
    "WM_RBUTTONDOWN",           /* 0x0204 */
    "WM_RBUTTONUP",             /* 0x0205 */
    "WM_RBUTTONDBLCLK",         /* 0x0206 */
    "WM_MBUTTONDOWN",           /* 0x0207 */
    "WM_MBUTTONUP",             /* 0x0208 */
    "WM_MBUTTONDBLCLK",         /* 0x0209 */
    "WM_MOUSEWHEEL",            /* 0x020A */
    "WM_XBUTTONDOWN",           /* 0x020B */
    "WM_XBUTTONUP",             /* 0x020C */
    "WM_XBUTTONDBLCLK",         /* 0x020D */
    NULL, NULL,

    "WM_PARENTNOTIFY",          /* 0x0210 */
    "WM_ENTERMENULOOP",         /* 0x0211 */
    "WM_EXITMENULOOP",          /* 0x0212 */
    "WM_NEXTMENU",              /* 0x0213 */
    "WM_SIZING",
    "WM_CAPTURECHANGED",
    "WM_MOVING", NULL,
    "WM_POWERBROADCAST",
    "WM_DEVICECHANGE", NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_MDICREATE",             /* 0x0220 */
    "WM_MDIDESTROY",            /* 0x0221 */
    "WM_MDIACTIVATE",           /* 0x0222 */
    "WM_MDIRESTORE",            /* 0x0223 */
    "WM_MDINEXT",               /* 0x0224 */
    "WM_MDIMAXIMIZE",           /* 0x0225 */
    "WM_MDITILE",               /* 0x0226 */
    "WM_MDICASCADE",            /* 0x0227 */
    "WM_MDIICONARRANGE",        /* 0x0228 */
    "WM_MDIGETACTIVE",          /* 0x0229 */

    "WM_DROPOBJECT",
    "WM_QUERYDROPOBJECT",
    "WM_BEGINDRAG",
    "WM_DRAGLOOP",
    "WM_DRAGSELECT",
    "WM_DRAGMOVE",

    /* 0x0230*/
    "WM_MDISETMENU",            /* 0x0230 */
    "WM_ENTERSIZEMOVE",         /* 0x0231 */
    "WM_EXITSIZEMOVE",          /* 0x0232 */
    "WM_DROPFILES",             /* 0x0233 */
    "WM_MDIREFRESHMENU", NULL, NULL, NULL,
    /* 0x0238*/
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0240 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0250 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0260 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0280 */
    NULL,
    "WM_IME_SETCONTEXT",        /* 0x0281 */
    "WM_IME_NOTIFY",            /* 0x0282 */
    "WM_IME_CONTROL",           /* 0x0283 */
    "WM_IME_COMPOSITIONFULL",   /* 0x0284 */
    "WM_IME_SELECT",            /* 0x0285 */
    "WM_IME_CHAR",              /* 0x0286 */
    NULL,
    "WM_IME_REQUEST",           /* 0x0288 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_IME_KEYDOWN",           /* 0x0290 */
    "WM_IME_KEYUP",             /* 0x0291 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x02a0 */
    "WM_NCMOUSEHOVER",          /* 0x02A0 */
    "WM_MOUSEHOVER",            /* 0x02A1 */
    "WM_NCMOUSELEAVE",          /* 0x02A2 */
    "WM_MOUSELEAVE",            /* 0x02A3 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_WTSSESSION_CHANGE",     /* 0x02B1 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x02c0 */
    "WM_TABLET_FIRST",          /* 0x02c0 */
    "WM_TABLET_FIRST+1",        /* 0x02c1 */
    "WM_TABLET_FIRST+2",        /* 0x02c2 */
    "WM_TABLET_FIRST+3",        /* 0x02c3 */
    "WM_TABLET_FIRST+4",        /* 0x02c4 */
    "WM_TABLET_FIRST+5",        /* 0x02c5 */
    "WM_TABLET_FIRST+7",        /* 0x02c6 */
    "WM_TABLET_FIRST+8",        /* 0x02c7 */
    "WM_TABLET_FIRST+9",        /* 0x02c8 */
    "WM_TABLET_FIRST+10",       /* 0x02c9 */
    "WM_TABLET_FIRST+11",       /* 0x02ca */
    "WM_TABLET_FIRST+12",       /* 0x02cb */
    "WM_TABLET_FIRST+13",       /* 0x02cc */
    "WM_TABLET_FIRST+14",       /* 0x02cd */
    "WM_TABLET_FIRST+15",       /* 0x02ce */
    "WM_TABLET_FIRST+16",       /* 0x02cf */
    "WM_TABLET_FIRST+17",       /* 0x02d0 */
    "WM_TABLET_FIRST+18",       /* 0x02d1 */
    "WM_TABLET_FIRST+19",       /* 0x02d2 */
    "WM_TABLET_FIRST+20",       /* 0x02d3 */
    "WM_TABLET_FIRST+21",       /* 0x02d4 */
    "WM_TABLET_FIRST+22",       /* 0x02d5 */
    "WM_TABLET_FIRST+23",       /* 0x02d6 */
    "WM_TABLET_FIRST+24",       /* 0x02d7 */
    "WM_TABLET_FIRST+25",       /* 0x02d8 */
    "WM_TABLET_FIRST+26",       /* 0x02d9 */
    "WM_TABLET_FIRST+27",       /* 0x02da */
    "WM_TABLET_FIRST+28",       /* 0x02db */
    "WM_TABLET_FIRST+29",       /* 0x02dc */
    "WM_TABLET_FIRST+30",       /* 0x02dd */
    "WM_TABLET_FIRST+31",       /* 0x02de */
    "WM_TABLET_LAST",           /* 0x02df */

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_CUT",                   /* 0x0300 */
    "WM_COPY",
    "WM_PASTE",
    "WM_CLEAR",
    "WM_UNDO",
    "WM_RENDERFORMAT",
    "WM_RENDERALLFORMATS",
    "WM_DESTROYCLIPBOARD",
    "WM_DRAWCLIPBOARD",
    "WM_PAINTCLIPBOARD",
    "WM_VSCROLLCLIPBOARD",
    "WM_SIZECLIPBOARD",
    "WM_ASKCBFORMATNAME",
    "WM_CHANGECBCHAIN",
    "WM_HSCROLLCLIPBOARD",
    "WM_QUERYNEWPALETTE",       /* 0x030f*/

    "WM_PALETTEISCHANGING",
    "WM_PALETTECHANGED",
    "WM_HOTKEY",                /* 0x0312 */
    "WM_POPUPSYSTEMMENU",       /* 0x0313 */
    NULL, NULL, NULL,
    "WM_PRINT",
    "WM_PRINTCLIENT",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0340 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_QUERYAFXWNDPROC",   /*  0x0360 */
    "WM_SIZEPARENT",        /*  0x0361 */
    "WM_SETMESSAGESTRING",  /*  0x0362 */
    "WM_IDLEUPDATECMDUI",   /*  0x0363 */
    "WM_INITIALUPDATE",     /*  0x0364 */
    "WM_COMMANDHELP",       /*  0x0365 */
    "WM_HELPHITTEST",       /*  0x0366 */
    "WM_EXITHELPMODE",      /*  0x0367 */
    "WM_RECALCPARENT",      /*  0x0368 */
    "WM_SIZECHILD",         /*  0x0369 */
    "WM_KICKIDLE",          /*  0x036A */
    "WM_QUERYCENTERWND",    /*  0x036B */
    "WM_DISABLEMODAL",      /*  0x036C */
    "WM_FLOATSTATUS",       /*  0x036D */
    "WM_ACTIVATETOPLEVEL",  /*  0x036E */
    "WM_QUERY3DCONTROLS",   /*  0x036F */
    NULL,NULL,NULL,
    "WM_SOCKET_NOTIFY",     /*  0x0373 */
    "WM_SOCKET_DEAD",       /*  0x0374 */
    "WM_POPMESSAGESTRING",  /*  0x0375 */
    "WM_OCC_LOADFROMSTREAM",     /* 0x0376 */
    "WM_OCC_LOADFROMSTORAGE",    /* 0x0377 */
    "WM_OCC_INITNEW",            /* 0x0378 */
    "WM_QUEUE_SENTINEL",         /* 0x0379 */
    "WM_OCC_LOADFROMSTREAM_EX",  /* 0x037A */
    "WM_OCC_LOADFROMSTORAGE_EX", /* 0x037B */

    NULL,NULL,NULL,NULL,

    "WM_PENWINFIRST",           /* 0x0380 */
    "WM_RCRESULT",              /* 0x0381 */
    "WM_HOOKRCRESULT",          /* 0x0382 */
    "WM_GLOBALRCCHANGE",        /* 0x0383 */
    "WM_SKB",                   /* 0x0384 */
    "WM_HEDITCTL",              /* 0x0385 */
    NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_PENWINLAST",            /* 0x038F */

    "WM_COALESCE_FIRST",        /* 0x0390 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_COALESCE_LAST",         /* 0x039F */

    /* 0x03a0 */
    "MM_JOY1MOVE",
    "MM_JOY2MOVE",
    "MM_JOY1ZMOVE",
    "MM_JOY2ZMOVE",
                            NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x03b0 */
    NULL, NULL, NULL, NULL, NULL,
    "MM_JOY1BUTTONDOWN",
    "MM_JOY2BUTTONDOWN",
    "MM_JOY1BUTTONUP",
    "MM_JOY2BUTTONUP",
    "MM_MCINOTIFY",
                NULL,
    "MM_WOM_OPEN",
    "MM_WOM_CLOSE",
    "MM_WOM_DONE",
    "MM_WIM_OPEN",
    "MM_WIM_CLOSE",

    /* 0x03c0 */
    "MM_WIM_DATA",
    "MM_MIM_OPEN",
    "MM_MIM_CLOSE",
    "MM_MIM_DATA",
    "MM_MIM_LONGDATA",
    "MM_MIM_ERROR",
    "MM_MIM_LONGERROR",
    "MM_MOM_OPEN",
    "MM_MOM_CLOSE",
    "MM_MOM_DONE",
                NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x03e0 */
    "WM_DDE_INITIATE",  /* 0x3E0 */
    "WM_DDE_TERMINATE", /* 0x3E1 */
    "WM_DDE_ADVISE",    /* 0x3E2 */
    "WM_DDE_UNADVISE",  /* 0x3E3 */
    "WM_DDE_ACK",       /* 0x3E4 */
    "WM_DDE_DATA",      /* 0x3E5 */
    "WM_DDE_REQUEST",   /* 0x3E6 */
    "WM_DDE_POKE",      /* 0x3E7 */
    "WM_DDE_EXECUTE",   /* 0x3E8 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,


    /* 0x03f0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_USER"                   /* 0x0400 */
};


#define SPY_MAX_LVMMSGNUM   140
static const char * const LVMMessageTypeNames[SPY_MAX_LVMMSGNUM + 1] =
{
    "LVM_GETBKCOLOR",           /* 1000 */
    "LVM_SETBKCOLOR",
    "LVM_GETIMAGELIST",
    "LVM_SETIMAGELIST",
    "LVM_GETITEMCOUNT",
    "LVM_GETITEMA",
    "LVM_SETITEMA",
    "LVM_INSERTITEMA",
    "LVM_DELETEITEM",
    "LVM_DELETEALLITEMS",
    "LVM_GETCALLBACKMASK",
    "LVM_SETCALLBACKMASK",
    "LVM_GETNEXTITEM",
    "LVM_FINDITEMA",
    "LVM_GETITEMRECT",
    "LVM_SETITEMPOSITION",
    "LVM_GETITEMPOSITION",
    "LVM_GETSTRINGWIDTHA",
    "LVM_HITTEST",
    "LVM_ENSUREVISIBLE",
    "LVM_SCROLL",
    "LVM_REDRAWITEMS",
    "LVM_ARRANGE",
    "LVM_EDITLABELA",
    "LVM_GETEDITCONTROL",
    "LVM_GETCOLUMNA",
    "LVM_SETCOLUMNA",
    "LVM_INSERTCOLUMNA",
    "LVM_DELETECOLUMN",
    "LVM_GETCOLUMNWIDTH",
    "LVM_SETCOLUMNWIDTH",
    "LVM_GETHEADER",
    NULL,
    "LVM_CREATEDRAGIMAGE",
    "LVM_GETVIEWRECT",
    "LVM_GETTEXTCOLOR",
    "LVM_SETTEXTCOLOR",
    "LVM_GETTEXTBKCOLOR",
    "LVM_SETTEXTBKCOLOR",
    "LVM_GETTOPINDEX",
    "LVM_GETCOUNTPERPAGE",
    "LVM_GETORIGIN",
    "LVM_UPDATE",
    "LVM_SETITEMSTATE",
    "LVM_GETITEMSTATE",
    "LVM_GETITEMTEXTA",
    "LVM_SETITEMTEXTA",
    "LVM_SETITEMCOUNT",
    "LVM_SORTITEMS",
    "LVM_SETITEMPOSITION32",
    "LVM_GETSELECTEDCOUNT",
    "LVM_GETITEMSPACING",
    "LVM_GETISEARCHSTRINGA",
    "LVM_SETICONSPACING",
    "LVM_SETEXTENDEDLISTVIEWSTYLE",
    "LVM_GETEXTENDEDLISTVIEWSTYLE",
    "LVM_GETSUBITEMRECT",
    "LVM_SUBITEMHITTEST",
    "LVM_SETCOLUMNORDERARRAY",
    "LVM_GETCOLUMNORDERARRAY",
    "LVM_SETHOTITEM",
    "LVM_GETHOTITEM",
    "LVM_SETHOTCURSOR",
    "LVM_GETHOTCURSOR",
    "LVM_APPROXIMATEVIEWRECT",
    "LVM_SETWORKAREAS",
    "LVM_GETSELECTIONMARK",
    "LVM_SETSELECTIONMARK",
    "LVM_SETBKIMAGEA",
    "LVM_GETBKIMAGEA",
    "LVM_GETWORKAREAS",
    "LVM_SETHOVERTIME",
    "LVM_GETHOVERTIME",
    "LVM_GETNUMBEROFWORKAREAS",
    "LVM_SETTOOLTIPS",
    "LVM_GETITEMW",
    "LVM_SETITEMW",
    "LVM_INSERTITEMW",
    "LVM_GETTOOLTIPS",
    NULL,
    NULL,
    NULL,
    NULL,
    "LVM_FINDITEMW",
    NULL,
    NULL,
    NULL,
    "LVM_GETSTRINGWIDTHW",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "LVM_GETCOLUMNW",
    "LVM_SETCOLUMNW",
    "LVM_INSERTCOLUMNW",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "LVM_GETITEMTEXTW",
    "LVM_SETITEMTEXTW",
    "LVM_GETISEARCHSTRINGW",
    "LVM_EDITLABELW",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "LVM_SETBKIMAGEW",
    "LVM_GETBKIMAGEW"   /* 0x108B */
};


#define SPY_MAX_TVMSGNUM   65
static const char * const TVMessageTypeNames[SPY_MAX_TVMSGNUM + 1] =
{
    "TVM_INSERTITEMA",          /* 1100 */
    "TVM_DELETEITEM",
    "TVM_EXPAND",
    NULL,
    "TVM_GETITEMRECT",
    "TVM_GETCOUNT",
    "TVM_GETINDENT",
    "TVM_SETINDENT",
    "TVM_GETIMAGELIST",
    "TVM_SETIMAGELIST",
    "TVM_GETNEXTITEM",
    "TVM_SELECTITEM",
    "TVM_GETITEMA",
    "TVM_SETITEMA",
    "TVM_EDITLABELA",
    "TVM_GETEDITCONTROL",
    "TVM_GETVISIBLECOUNT",
    "TVM_HITTEST",
    "TVM_CREATEDRAGIMAGE",
    "TVM_SORTCHILDREN",
    "TVM_ENSUREVISIBLE",
    "TVM_SORTCHILDRENCB",
    "TVM_ENDEDITLABELNOW",
    "TVM_GETISEARCHSTRINGA",
    "TVM_SETTOOLTIPS",
    "TVM_GETTOOLTIPS",
    "TVM_SETINSERTMARK",
    "TVM_SETITEMHEIGHT",
    "TVM_GETITEMHEIGHT",
    "TVM_SETBKCOLOR",
    "TVM_SETTEXTCOLOR",
    "TVM_GETBKCOLOR",
    "TVM_GETTEXTCOLOR",
    "TVM_SETSCROLLTIME",
    "TVM_GETSCROLLTIME",
    "TVM_UNKNOWN35",
    "TVM_UNKNOWN36",
    "TVM_SETINSERTMARKCOLOR",
    "TVM_GETINSERTMARKCOLOR",
    "TVM_GETITEMSTATE",
    "TVM_SETLINECOLOR",
    "TVM_GETLINECOLOR",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "TVM_INSERTITEMW",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "TVM_GETITEMW",
    "TVM_SETITEMW",
    "TVM_GETISEARCHSTRINGW",
    "TVM_EDITLABELW"
};


#define SPY_MAX_HDMMSGNUM   19
static const char * const HDMMessageTypeNames[SPY_MAX_HDMMSGNUM + 1] =
{
    "HDM_GETITEMCOUNT",         /* 1200 */
    "HDM_INSERTITEMA",
    "HDM_DELETEITEM",
    "HDM_GETITEMA",
    "HDM_SETITEMA",
    "HDM_LAYOUT",
    "HDM_HITTEST",
    "HDM_GETITEMRECT",
    "HDM_SETIMAGELIST",
    "HDM_GETIMAGELIST",
    "HDM_INSERTITEMW",
    "HDM_GETITEMW",
    "HDM_SETITEMW",
    NULL,
    NULL,
    "HDM_ORDERTOINDEX",
    "HDM_CREATEDRAGIMAGE",
    "GETORDERARRAYINDEX",
    "SETORDERARRAYINDEX",
    "SETHOTDIVIDER"
};


#define SPY_MAX_TCMMSGNUM   62
static const char * const TCMMessageTypeNames[SPY_MAX_TCMMSGNUM + 1] =
{
    NULL,               /* 1300 */
    NULL,
    "TCM_SETIMAGELIST",
    "TCM_GETIMAGELIST",
    "TCM_GETITEMCOUNT",
    "TCM_GETITEMA",
    "TCM_SETITEMA",
    "TCM_INSERTITEMA",
    "TCM_DELETEITEM",
    "TCM_DELETEALLITEMS",
    "TCM_GETITEMRECT",
    "TCM_GETCURSEL",
    "TCM_SETCURSEL",
    "TCM_HITTEST",
    "TCM_SETITEMEXTRA",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "TCM_ADJUSTRECT",
    "TCM_SETITEMSIZE",
    "TCM_REMOVEIMAGE",
    "TCM_SETPADDING",
    "TCM_GETROWCOUNT",
    "TCM_GETTOOLTIPS",
    "TCM_SETTOOLTIPS",
    "TCM_GETCURFOCUS",
    "TCM_SETCURFOCUS",
    "TCM_SETMINTABWIDTH",
    "TCM_DESELECTALL",
    "TCM_HIGHLIGHTITEM",
    "TCM_SETEXTENDEDSTYLE",
    "TCM_GETEXTENDEDSTYLE",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "TCM_GETITEMW",
    "TCM_SETITEMW",
    "TCM_INSERTITEMW"
};

#define SPY_MAX_PGMMSGNUM   13
static const char * const PGMMessageTypeNames[SPY_MAX_PGMMSGNUM + 1] =
{
    NULL,               /* 1400 */
    "PGM_SETCHILD",
    "PGM_RECALCSIZE",
    "PGM_FORWARDMOUSE",
    "PGM_SETBKCOLOR",
    "PGM_GETBKCOLOR",
    "PGM_SETBORDER",
    "PGM_GETBORDER",
    "PGM_SETPOS",
    "PGM_GETPOS",
    "PGM_SETBUTTONSIZE",
    "PGM_GETBUTTONSIZE",
    "PGM_GETBUTTONSTATE",
    "PGM_GETDROPTARGET"
};


#define SPY_MAX_CCMMSGNUM   9
static const char * const CCMMessageTypeNames[SPY_MAX_CCMMSGNUM + 1] =
{
    NULL,               /* 0x2000 */
    "CCM_SETBKCOLOR",
    "CCM_SETCOLORSCHEME",
    "CCM_GETCOLORSCHEME",
    "CCM_GETDROPTARGET",
    "CCM_SETUNICODEFORMAT",
    "CCM_GETUNICODEFORMAT",
    "CCM_SETVERSION",
    "CCM_GETVERSION",
    "CCM_SETNOTIFYWINDOW"
};

#define SPY_MAX_WINEMSGNUM   6
static const char * const WINEMessageTypeNames[SPY_MAX_WINEMSGNUM + 1] =
{
    "WM_WINE_DESTROYWINDOW",
    "WM_WINE_SETWINDOWPOS",
    "WM_WINE_SHOWWINDOW",
    "WM_WINE_SETPARENT",
    "WM_WINE_SETWINDOWLONG",
    "WM_WINE_ENABLEWINDOW"
};

/* Virtual key names */
#define SPY_MAX_VKKEYSNUM 255
static const char * const VK_KeyNames[SPY_MAX_VKKEYSNUM + 1] =
{
    NULL,               /* 0x00 */
    "VK_LBUTTON",       /* 0x01 */
    "VK_RBUTTON",       /* 0x02 */
    "VK_CANCEL",        /* 0x03 */
    "VK_MBUTTON",       /* 0x04 */
    "VK_XBUTTON1",      /* 0x05 */
    "VK_XBUTTON2",      /* 0x06 */
    NULL,               /* 0x07 */
    "VK_BACK",          /* 0x08 */
    "VK_TAB",           /* 0x09 */
    NULL,               /* 0x0A */
    NULL,               /* 0x0B */
    "VK_CLEAR",         /* 0x0C */
    "VK_RETURN",        /* 0x0D */
    NULL,               /* 0x0E */
    NULL,               /* 0x0F */
    "VK_SHIFT",         /* 0x10 */
    "VK_CONTROL",       /* 0x11 */
    "VK_MENU",          /* 0x12 */
    "VK_PAUSE",         /* 0x13 */
    "VK_CAPITAL",       /* 0x14 */
    NULL,               /* 0x15 */
    NULL,               /* 0x16 */
    NULL,               /* 0x17 */
    NULL,               /* 0x18 */
    NULL,               /* 0x19 */
    NULL,               /* 0x1A */
    "VK_ESCAPE",        /* 0x1B */
    NULL,               /* 0x1C */
    NULL,               /* 0x1D */
    NULL,               /* 0x1E */
    NULL,               /* 0x1F */
    "VK_SPACE",         /* 0x20 */
    "VK_PRIOR",         /* 0x21 */
    "VK_NEXT",          /* 0x22 */
    "VK_END",           /* 0x23 */
    "VK_HOME",          /* 0x24 */
    "VK_LEFT",          /* 0x25 */
    "VK_UP",            /* 0x26 */
    "VK_RIGHT",         /* 0x27 */
    "VK_DOWN",          /* 0x28 */
    "VK_SELECT",        /* 0x29 */
    "VK_PRINT",         /* 0x2A */
    "VK_EXECUTE",       /* 0x2B */
    "VK_SNAPSHOT",      /* 0x2C */
    "VK_INSERT",        /* 0x2D */
    "VK_DELETE",        /* 0x2E */
    "VK_HELP",          /* 0x2F */
    "VK_0",             /* 0x30 */
    "VK_1",             /* 0x31 */
    "VK_2",             /* 0x32 */
    "VK_3",             /* 0x33 */
    "VK_4",             /* 0x34 */
    "VK_5",             /* 0x35 */
    "VK_6",             /* 0x36 */
    "VK_7",             /* 0x37 */
    "VK_8",             /* 0x38 */
    "VK_9",             /* 0x39 */
    NULL,               /* 0x3A */
    NULL,               /* 0x3B */
    NULL,               /* 0x3C */
    NULL,               /* 0x3D */
    NULL,               /* 0x3E */
    NULL,               /* 0x3F */
    NULL,               /* 0x40 */
    "VK_A",             /* 0x41 */
    "VK_B",             /* 0x42 */
    "VK_C",             /* 0x43 */
    "VK_D",             /* 0x44 */
    "VK_E",             /* 0x45 */
    "VK_F",             /* 0x46 */
    "VK_G",             /* 0x47 */
    "VK_H",             /* 0x48 */
    "VK_I",             /* 0x49 */
    "VK_J",             /* 0x4A */
    "VK_K",             /* 0x4B */
    "VK_L",             /* 0x4C */
    "VK_M",             /* 0x4D */
    "VK_N",             /* 0x4E */
    "VK_O",             /* 0x4F */
    "VK_P",             /* 0x50 */
    "VK_Q",             /* 0x51 */
    "VK_R",             /* 0x52 */
    "VK_S",             /* 0x53 */
    "VK_T",             /* 0x54 */
    "VK_U",             /* 0x55 */
    "VK_V",             /* 0x56 */
    "VK_W",             /* 0x57 */
    "VK_X",             /* 0x58 */
    "VK_Y",             /* 0x59 */
    "VK_Z",             /* 0x5A */
    "VK_LWIN",          /* 0x5B */
    "VK_RWIN",          /* 0x5C */
    "VK_APPS",          /* 0x5D */
    NULL,               /* 0x5E */
    NULL,               /* 0x5F */
    "VK_NUMPAD0",       /* 0x60 */
    "VK_NUMPAD1",       /* 0x61 */
    "VK_NUMPAD2",       /* 0x62 */
    "VK_NUMPAD3",       /* 0x63 */
    "VK_NUMPAD4",       /* 0x64 */
    "VK_NUMPAD5",       /* 0x65 */
    "VK_NUMPAD6",       /* 0x66 */
    "VK_NUMPAD7",       /* 0x67 */
    "VK_NUMPAD8",       /* 0x68 */
    "VK_NUMPAD9",       /* 0x69 */
    "VK_MULTIPLY",      /* 0x6A */
    "VK_ADD",           /* 0x6B */
    "VK_SEPARATOR",     /* 0x6C */
    "VK_SUBTRACT",      /* 0x6D */
    "VK_DECIMAL",       /* 0x6E */
    "VK_DIVIDE",        /* 0x6F */
    "VK_F1",            /* 0x70 */
    "VK_F2",            /* 0x71 */
    "VK_F3",            /* 0x72 */
    "VK_F4",            /* 0x73 */
    "VK_F5",            /* 0x74 */
    "VK_F6",            /* 0x75 */
    "VK_F7",            /* 0x76 */
    "VK_F8",            /* 0x77 */
    "VK_F9",            /* 0x78 */
    "VK_F10",           /* 0x79 */
    "VK_F11",           /* 0x7A */
    "VK_F12",           /* 0x7B */
    "VK_F13",           /* 0x7C */
    "VK_F14",           /* 0x7D */
    "VK_F15",           /* 0x7E */
    "VK_F16",           /* 0x7F */
    "VK_F17",           /* 0x80 */
    "VK_F18",           /* 0x81 */
    "VK_F19",           /* 0x82 */
    "VK_F20",           /* 0x83 */
    "VK_F21",           /* 0x84 */
    "VK_F22",           /* 0x85 */
    "VK_F23",           /* 0x86 */
    "VK_F24",           /* 0x87 */
    NULL,               /* 0x88 */
    NULL,               /* 0x89 */
    NULL,               /* 0x8A */
    NULL,               /* 0x8B */
    NULL,               /* 0x8C */
    NULL,               /* 0x8D */
    NULL,               /* 0x8E */
    NULL,               /* 0x8F */
    "VK_NUMLOCK",       /* 0x90 */
    "VK_SCROLL",        /* 0x91 */
    NULL,               /* 0x92 */
    NULL,               /* 0x93 */
    NULL,               /* 0x94 */
    NULL,               /* 0x95 */
    NULL,               /* 0x96 */
    NULL,               /* 0x97 */
    NULL,               /* 0x98 */
    NULL,               /* 0x99 */
    NULL,               /* 0x9A */
    NULL,               /* 0x9B */
    NULL,               /* 0x9C */
    NULL,               /* 0x9D */
    NULL,               /* 0x9E */
    NULL,               /* 0x9F */
    "VK_LSHIFT",        /* 0xA0 */
    "VK_RSHIFT",        /* 0xA1 */
    "VK_LCONTROL",      /* 0xA2 */
    "VK_RCONTROL",      /* 0xA3 */
    "VK_LMENU",         /* 0xA4 */
    "VK_RMENU",         /* 0xA5 */
    NULL,               /* 0xA6 */
    NULL,               /* 0xA7 */
    NULL,               /* 0xA8 */
    NULL,               /* 0xA9 */
    NULL,               /* 0xAA */
    NULL,               /* 0xAB */
    NULL,               /* 0xAC */
    NULL,               /* 0xAD */
    NULL,               /* 0xAE */
    NULL,               /* 0xAF */
    NULL,               /* 0xB0 */
    NULL,               /* 0xB1 */
    NULL,               /* 0xB2 */
    NULL,               /* 0xB3 */
    NULL,               /* 0xB4 */
    NULL,               /* 0xB5 */
    NULL,               /* 0xB6 */
    NULL,               /* 0xB7 */
    NULL,               /* 0xB8 */
    NULL,               /* 0xB9 */
    "VK_OEM_1",         /* 0xBA */
    "VK_OEM_PLUS",      /* 0xBB */
    "VK_OEM_COMMA",     /* 0xBC */
    "VK_OEM_MINUS",     /* 0xBD */
    "VK_OEM_PERIOD",    /* 0xBE */
    "VK_OEM_2",         /* 0xBF */
    "VK_OEM_3",         /* 0xC0 */
    NULL,               /* 0xC1 */
    NULL,               /* 0xC2 */
    NULL,               /* 0xC3 */
    NULL,               /* 0xC4 */
    NULL,               /* 0xC5 */
    NULL,               /* 0xC6 */
    NULL,               /* 0xC7 */
    NULL,               /* 0xC8 */
    NULL,               /* 0xC9 */
    NULL,               /* 0xCA */
    NULL,               /* 0xCB */
    NULL,               /* 0xCC */
    NULL,               /* 0xCD */
    NULL,               /* 0xCE */
    NULL,               /* 0xCF */
    NULL,               /* 0xD0 */
    NULL,               /* 0xD1 */
    NULL,               /* 0xD2 */
    NULL,               /* 0xD3 */
    NULL,               /* 0xD4 */
    NULL,               /* 0xD5 */
    NULL,               /* 0xD6 */
    NULL,               /* 0xD7 */
    NULL,               /* 0xD8 */
    NULL,               /* 0xD9 */
    NULL,               /* 0xDA */
    "VK_OEM_4",         /* 0xDB */
    "VK_OEM_5",         /* 0xDC */
    "VK_OEM_6",         /* 0xDD */
    "VK_OEM_7",         /* 0xDE */
    "VK_OEM_8",         /* 0xDF */
    NULL,               /* 0xE0 */
    "VK_OEM_AX",        /* 0xE1 */
    "VK_OEM_102",       /* 0xE2 */
    "VK_ICO_HELP",      /* 0xE3 */
    "VK_ICO_00",        /* 0xE4 */
    "VK_PROCESSKEY",    /* 0xE5 */
    NULL,               /* 0xE6 */
    NULL,               /* 0xE7 */
    NULL,               /* 0xE8 */
    NULL,               /* 0xE9 */
    NULL,               /* 0xEA */
    NULL,               /* 0xEB */
    NULL,               /* 0xEC */
    NULL,               /* 0xED */
    NULL,               /* 0xEE */
    NULL,               /* 0xEF */
    NULL,               /* 0xF0 */
    NULL,               /* 0xF1 */
    NULL,               /* 0xF2 */
    NULL,               /* 0xF3 */
    NULL,               /* 0xF4 */
    NULL,               /* 0xF5 */
    "VK_ATTN",          /* 0xF6 */
    "VK_CRSEL",         /* 0xF7 */
    "VK_EXSEL",         /* 0xF8 */
    "VK_EREOF",         /* 0xF9 */
    "VK_PLAY",          /* 0xFA */
    "VK_ZOOM",          /* 0xFB */
    "VK_NONAME",        /* 0xFC */
    "VK_PA1",           /* 0xFD */
    "VK_OEM_CLEAR",     /* 0xFE */
    NULL                /* 0xFF */
};


/************************************************************************/


/* WM_USER+n message values for "common controls" */

typedef struct
{
    const char *name;      /* name of control message           */
    UINT        value;     /* message number (0x0401-0x0fff     */
    UINT        len;       /* length of space at lParam to dump */
} USER_MSG;


typedef struct
{
const WCHAR      *classname;  /* class name to match                  */
const USER_MSG   *classmsg;   /* pointer to first USER_MSG for class  */
const USER_MSG   *lastmsg;    /* pointer to last USER_MSG for class   */
} CONTROL_CLASS;

#define USM(a,b) { #a ,a,b}
#define SZOF(a)  sizeof(a)

/* To dump memory at the lParam for any of these messages,  */
/* replace the "0" with a "SZOF(structure)", or with a      */
/* number. (First method preferred.)                         */

#define RB_GETBANDINFO_OLD (WM_USER+5) /* obsoleted after IE3, but we have to support it anyway */

static const USER_MSG rebar_array[] = {
          USM(RB_INSERTBANDA,          0),
          USM(RB_DELETEBAND,           0),
          USM(RB_GETBARINFO,           0),
          USM(RB_SETBARINFO,           0),
          USM(RB_GETBANDINFO_OLD,      0),
          USM(RB_SETBANDINFOA,         0),
          USM(RB_SETPARENT,            0),
          USM(RB_HITTEST,              0),
          USM(RB_GETRECT,              0),
          USM(RB_INSERTBANDW,          0),
          USM(RB_SETBANDINFOW,         0),
          USM(RB_GETBANDCOUNT,         0),
          USM(RB_GETROWCOUNT,          0),
          USM(RB_GETROWHEIGHT,         0),
          USM(RB_IDTOINDEX,            0),
          USM(RB_GETTOOLTIPS,          0),
          USM(RB_SETTOOLTIPS,          0),
          USM(RB_SETBKCOLOR,           0),
          USM(RB_GETBKCOLOR,           0),
          USM(RB_SETTEXTCOLOR,         0),
          USM(RB_GETTEXTCOLOR,         0),
          USM(RB_SIZETORECT,           0),
          USM(RB_BEGINDRAG,            0),
          USM(RB_ENDDRAG,              0),
          USM(RB_DRAGMOVE,             0),
          USM(RB_GETBARHEIGHT,         0),
          USM(RB_GETBANDINFOW,         0),
          USM(RB_GETBANDINFOA,         0),
          USM(RB_MINIMIZEBAND,         0),
          USM(RB_MAXIMIZEBAND,         0),
          USM(RB_GETBANDBORDERS,       0),
          USM(RB_SHOWBAND,             0),
          USM(RB_SETPALETTE,           0),
          USM(RB_GETPALETTE,           0),
          USM(RB_MOVEBAND,             0),
          {0,0,0} };

static const USER_MSG toolbar_array[] = {
          USM(TB_ENABLEBUTTON          ,0),
          USM(TB_CHECKBUTTON           ,0),
          USM(TB_PRESSBUTTON           ,0),
          USM(TB_HIDEBUTTON            ,0),
          USM(TB_INDETERMINATE         ,0),
          USM(TB_MARKBUTTON            ,0),
          USM(TB_ISBUTTONENABLED       ,0),
          USM(TB_ISBUTTONCHECKED       ,0),
          USM(TB_ISBUTTONPRESSED       ,0),
          USM(TB_ISBUTTONHIDDEN        ,0),
          USM(TB_ISBUTTONINDETERMINATE ,0),
          USM(TB_ISBUTTONHIGHLIGHTED   ,0),
          USM(TB_SETSTATE              ,0),
          USM(TB_GETSTATE              ,0),
          USM(TB_ADDBITMAP             ,0),
          USM(TB_ADDBUTTONSA           ,0),
          USM(TB_INSERTBUTTONA         ,0),
          USM(TB_DELETEBUTTON          ,0),
          USM(TB_GETBUTTON             ,0),
          USM(TB_BUTTONCOUNT           ,0),
          USM(TB_COMMANDTOINDEX        ,0),
          USM(TB_SAVERESTOREA          ,0),
          USM(TB_CUSTOMIZE             ,0),
          USM(TB_ADDSTRINGA            ,0),
          USM(TB_GETITEMRECT           ,0),
          USM(TB_BUTTONSTRUCTSIZE      ,0),
          USM(TB_SETBUTTONSIZE         ,0),
          USM(TB_SETBITMAPSIZE         ,0),
          USM(TB_AUTOSIZE              ,0),
          USM(TB_GETTOOLTIPS           ,0),
          USM(TB_SETTOOLTIPS           ,0),
          USM(TB_SETPARENT             ,0),
          USM(TB_SETROWS               ,0),
          USM(TB_GETROWS               ,0),
          USM(TB_GETBITMAPFLAGS        ,0),
          USM(TB_SETCMDID              ,0),
          USM(TB_CHANGEBITMAP          ,0),
          USM(TB_GETBITMAP             ,0),
          USM(TB_GETBUTTONTEXTA        ,0),
          USM(TB_REPLACEBITMAP         ,0),
          USM(TB_SETINDENT             ,0),
          USM(TB_SETIMAGELIST          ,0),
          USM(TB_GETIMAGELIST          ,0),
          USM(TB_LOADIMAGES            ,0),
          USM(TB_GETRECT               ,0),
          USM(TB_SETHOTIMAGELIST       ,0),
          USM(TB_GETHOTIMAGELIST       ,0),
          USM(TB_SETDISABLEDIMAGELIST  ,0),
          USM(TB_GETDISABLEDIMAGELIST  ,0),
          USM(TB_SETSTYLE              ,0),
          USM(TB_GETSTYLE              ,0),
          USM(TB_GETBUTTONSIZE         ,0),
          USM(TB_SETBUTTONWIDTH        ,0),
          USM(TB_SETMAXTEXTROWS        ,0),
          USM(TB_GETTEXTROWS           ,0),
          USM(TB_GETOBJECT             ,0),
          USM(TB_GETBUTTONINFOW        ,0),
          USM(TB_SETBUTTONINFOW        ,0),
          USM(TB_GETBUTTONINFOA        ,0),
          USM(TB_SETBUTTONINFOA        ,0),
          USM(TB_INSERTBUTTONW         ,0),
          USM(TB_ADDBUTTONSW           ,0),
          USM(TB_HITTEST               ,0),
          USM(TB_SETDRAWTEXTFLAGS      ,0),
          USM(TB_GETHOTITEM            ,0),
          USM(TB_SETHOTITEM            ,0),
          USM(TB_SETANCHORHIGHLIGHT    ,0),
          USM(TB_GETANCHORHIGHLIGHT    ,0),
          USM(TB_GETBUTTONTEXTW        ,0),
          USM(TB_SAVERESTOREW          ,0),
          USM(TB_ADDSTRINGW            ,0),
          USM(TB_MAPACCELERATORA       ,0),
          USM(TB_GETINSERTMARK         ,0),
          USM(TB_SETINSERTMARK         ,0),
          USM(TB_INSERTMARKHITTEST     ,0),
          USM(TB_MOVEBUTTON            ,0),
          USM(TB_GETMAXSIZE            ,0),
          USM(TB_SETEXTENDEDSTYLE      ,0),
          USM(TB_GETEXTENDEDSTYLE      ,0),
          USM(TB_GETPADDING            ,0),
          USM(TB_SETPADDING            ,0),
          USM(TB_SETINSERTMARKCOLOR    ,0),
          USM(TB_GETINSERTMARKCOLOR    ,0),
          USM(TB_MAPACCELERATORW       ,0),
          USM(TB_GETSTRINGW            ,0),
          USM(TB_GETSTRINGA            ,0),
          USM(TB_UNKWN45D              ,8),
          USM(TB_SETHOTITEM2           ,0),
          USM(TB_SETLISTGAP            ,0),
          USM(TB_GETIMAGELISTCOUNT     ,0),
          USM(TB_GETIDEALSIZE          ,0),
          {0,0,0} };

static const USER_MSG tooltips_array[] = {
          USM(TTM_ACTIVATE             ,0),
          USM(TTM_SETDELAYTIME         ,0),
          USM(TTM_ADDTOOLA             ,0),
          USM(TTM_DELTOOLA             ,0),
          USM(TTM_NEWTOOLRECTA         ,0),
          USM(TTM_RELAYEVENT           ,0),
          USM(TTM_GETTOOLINFOA         ,0),
          USM(TTM_HITTESTA             ,0),
          USM(TTM_GETTEXTA             ,0),
          USM(TTM_UPDATETIPTEXTA       ,0),
          USM(TTM_GETTOOLCOUNT         ,0),
          USM(TTM_ENUMTOOLSA           ,0),
          USM(TTM_GETCURRENTTOOLA      ,0),
          USM(TTM_WINDOWFROMPOINT      ,0),
          USM(TTM_TRACKACTIVATE        ,0),
          USM(TTM_TRACKPOSITION        ,0),
          USM(TTM_SETTIPBKCOLOR        ,0),
          USM(TTM_SETTIPTEXTCOLOR      ,0),
          USM(TTM_GETDELAYTIME         ,0),
          USM(TTM_GETTIPBKCOLOR        ,0),
          USM(TTM_GETTIPTEXTCOLOR      ,0),
          USM(TTM_SETMAXTIPWIDTH       ,0),
          USM(TTM_GETMAXTIPWIDTH       ,0),
          USM(TTM_SETMARGIN            ,0),
          USM(TTM_GETMARGIN            ,0),
          USM(TTM_POP                  ,0),
          USM(TTM_UPDATE               ,0),
          USM(TTM_GETBUBBLESIZE        ,0),
          USM(TTM_ADDTOOLW             ,0),
          USM(TTM_DELTOOLW             ,0),
          USM(TTM_NEWTOOLRECTW         ,0),
          USM(TTM_GETTOOLINFOW         ,0),
          USM(TTM_SETTOOLINFOW         ,0),
          USM(TTM_HITTESTW             ,0),
          USM(TTM_GETTEXTW             ,0),
          USM(TTM_UPDATETIPTEXTW       ,0),
          USM(TTM_ENUMTOOLSW           ,0),
          USM(TTM_GETCURRENTTOOLW      ,0),
          {0,0,0} };

static const USER_MSG comboex_array[] = {
          USM(CBEM_INSERTITEMA        ,0),
          USM(CBEM_SETIMAGELIST       ,0),
          USM(CBEM_GETIMAGELIST       ,0),
          USM(CBEM_GETITEMA           ,0),
          USM(CBEM_SETITEMA           ,0),
          USM(CBEM_GETCOMBOCONTROL    ,0),
          USM(CBEM_GETEDITCONTROL     ,0),
          USM(CBEM_SETEXSTYLE         ,0),
          USM(CBEM_GETEXTENDEDSTYLE   ,0),
          USM(CBEM_HASEDITCHANGED     ,0),
          USM(CBEM_INSERTITEMW        ,0),
          USM(CBEM_SETITEMW           ,0),
          USM(CBEM_GETITEMW           ,0),
          USM(CBEM_SETEXTENDEDSTYLE   ,0),
          {0,0,0} };

static const USER_MSG propsht_array[] = {
          USM(PSM_SETCURSEL           ,0),
          USM(PSM_REMOVEPAGE          ,0),
          USM(PSM_ADDPAGE             ,0),
          USM(PSM_CHANGED             ,0),
          USM(PSM_RESTARTWINDOWS      ,0),
          USM(PSM_REBOOTSYSTEM        ,0),
          USM(PSM_CANCELTOCLOSE       ,0),
          USM(PSM_QUERYSIBLINGS       ,0),
          USM(PSM_UNCHANGED           ,0),
          USM(PSM_APPLY               ,0),
          USM(PSM_SETTITLEA           ,0),
          USM(PSM_SETWIZBUTTONS       ,0),
          USM(PSM_PRESSBUTTON         ,0),
          USM(PSM_SETCURSELID         ,0),
          USM(PSM_SETFINISHTEXTA      ,0),
          USM(PSM_GETTABCONTROL       ,0),
          USM(PSM_ISDIALOGMESSAGE     ,0),
          USM(PSM_GETCURRENTPAGEHWND  ,0),
          USM(PSM_SETTITLEW           ,0),
          USM(PSM_SETFINISHTEXTW      ,0),
          {0,0,0} };
const WCHAR PropSheetInfoStr[] =
    {'P','r','o','p','e','r','t','y','S','h','e','e','t','I','n','f','o',0 };

static const USER_MSG updown_array[] = {
          USM(UDM_SETRANGE            ,0),
          USM(UDM_GETRANGE            ,0),
          USM(UDM_SETPOS              ,0),
          USM(UDM_GETPOS              ,0),
          USM(UDM_SETBUDDY            ,0),
          USM(UDM_GETBUDDY            ,0),
          USM(UDM_SETACCEL            ,0),
          USM(UDM_GETACCEL            ,0),
          USM(UDM_SETBASE             ,0),
          USM(UDM_GETBASE             ,0),
          USM(UDM_SETRANGE32          ,0),
          USM(UDM_GETRANGE32          ,0),
          USM(UDM_SETPOS32            ,0),
          USM(UDM_GETPOS32            ,0),
          {0,0,0} };

/* generated from:
 * $ for i in `grep EM_ include/richedit.h | cut -d' ' -f2 | cut -f1`; do echo -e "          USM($i\t\t,0),"; done
 */
static const USER_MSG richedit_array[] = {
          {"EM_SCROLLCARET", WM_USER+49 ,0},
          USM(EM_CANPASTE               ,0),
          USM(EM_DISPLAYBAND            ,0),
          USM(EM_EXGETSEL               ,0),
          USM(EM_EXLIMITTEXT            ,0),
          USM(EM_EXLINEFROMCHAR         ,0),
          USM(EM_EXSETSEL               ,0),
          USM(EM_FINDTEXT               ,0),
          USM(EM_FORMATRANGE            ,0),
          USM(EM_GETCHARFORMAT          ,0),
          USM(EM_GETEVENTMASK           ,0),
          USM(EM_GETOLEINTERFACE        ,0),
          USM(EM_GETPARAFORMAT          ,0),
          USM(EM_GETSELTEXT             ,0),
          USM(EM_HIDESELECTION          ,0),
          USM(EM_PASTESPECIAL           ,0),
          USM(EM_REQUESTRESIZE          ,0),
          USM(EM_SELECTIONTYPE          ,0),
          USM(EM_SETBKGNDCOLOR          ,0),
          USM(EM_SETCHARFORMAT          ,0),
          USM(EM_SETEVENTMASK           ,0),
          USM(EM_SETOLECALLBACK         ,0),
          USM(EM_SETPARAFORMAT          ,0),
          USM(EM_SETTARGETDEVICE        ,0),
          USM(EM_STREAMIN               ,0),
          USM(EM_STREAMOUT              ,0),
          USM(EM_GETTEXTRANGE           ,0),
          USM(EM_FINDWORDBREAK          ,0),
          USM(EM_SETOPTIONS             ,0),
          USM(EM_GETOPTIONS             ,0),
          USM(EM_FINDTEXTEX             ,0),
          USM(EM_GETWORDBREAKPROCEX     ,0),
          USM(EM_SETWORDBREAKPROCEX     ,0),
          USM(EM_SETUNDOLIMIT           ,0),
          USM(EM_REDO                   ,0),
          USM(EM_CANREDO                ,0),
          USM(EM_GETUNDONAME            ,0),
          USM(EM_GETREDONAME            ,0),
          USM(EM_STOPGROUPTYPING        ,0),
          USM(EM_SETTEXTMODE            ,0),
          USM(EM_GETTEXTMODE            ,0),
          USM(EM_AUTOURLDETECT          ,0),
          USM(EM_GETAUTOURLDETECT       ,0),
          USM(EM_SETPALETTE             ,0),
          USM(EM_GETTEXTEX              ,0),
          USM(EM_GETTEXTLENGTHEX        ,0),
          USM(EM_SHOWSCROLLBAR          ,0),
          USM(EM_SETTEXTEX              ,0),
          USM(EM_SETPUNCTUATION         ,0),
          USM(EM_GETPUNCTUATION         ,0),
          USM(EM_SETWORDWRAPMODE        ,0),
          USM(EM_GETWORDWRAPMODE        ,0),
          USM(EM_SETIMECOLOR            ,0),
          USM(EM_GETIMECOLOR            ,0),
          USM(EM_SETIMEOPTIONS          ,0),
          USM(EM_GETIMEOPTIONS          ,0),
          USM(EM_CONVPOSITION           ,0),
          USM(EM_SETLANGOPTIONS         ,0),
          USM(EM_GETLANGOPTIONS         ,0),
          USM(EM_GETIMECOMPMODE         ,0),
          USM(EM_FINDTEXTW              ,0),
          USM(EM_FINDTEXTEXW            ,0),
          USM(EM_RECONVERSION           ,0),
          USM(EM_SETIMEMODEBIAS         ,0),
          USM(EM_GETIMEMODEBIAS         ,0),
          USM(EM_SETBIDIOPTIONS         ,0),
          USM(EM_GETBIDIOPTIONS         ,0),
          USM(EM_SETTYPOGRAPHYOPTIONS   ,0),
          USM(EM_GETTYPOGRAPHYOPTIONS   ,0),
          USM(EM_SETEDITSTYLE           ,0),
          USM(EM_GETEDITSTYLE           ,0),
          USM(EM_OUTLINE                ,0),
          USM(EM_GETSCROLLPOS           ,0),
          USM(EM_SETSCROLLPOS           ,0),
          USM(EM_SETFONTSIZE            ,0),
          USM(EM_GETZOOM                ,0),
          USM(EM_SETZOOM                ,0),
          {0,0,0} };

#undef SZOF
#undef USM

static CONTROL_CLASS  cc_array[] = {
    {WC_COMBOBOXEXW,    comboex_array,  0},
    {WC_PROPSHEETW,     propsht_array,  0},
    {REBARCLASSNAMEW,   rebar_array,    0},
    {TOOLBARCLASSNAMEW, toolbar_array,  0},
    {TOOLTIPS_CLASSW,   tooltips_array, 0},
    {UPDOWN_CLASSW,     updown_array,   0},
    {RICHEDIT_CLASS20W, richedit_array, 0},
    {0, 0, 0} };


/************************************************************************/


/* WM_NOTIFY function codes display */

typedef struct
{
    const char *name;     /* name of notify message        */
    UINT        value;     /* notify code value             */
    UINT        len;       /* length of extra space to dump */
} SPY_NOTIFY;

#define SPNFY(a,b) { #a ,a,sizeof(b)-sizeof(NMHDR)}

/* Array MUST be in descending order by the 'value' field  */
/* (since value is UNSIGNED, 0xffffffff is largest and     */
/*  0xfffffffe is smaller). A binary search is used to     */
/* locate the correct 'value'.                             */
static const SPY_NOTIFY spnfy_array[] = {
    /*  common        0U       to  0U-99U  */
    SPNFY(NM_OUTOFMEMORY,        NMHDR),
    SPNFY(NM_CLICK,              NMHDR),
    SPNFY(NM_DBLCLK,             NMHDR),
    SPNFY(NM_RETURN,             NMHDR),
    SPNFY(NM_RCLICK,             NMHDR),
    SPNFY(NM_RDBLCLK,            NMHDR),
    SPNFY(NM_SETFOCUS,           NMHDR),
    SPNFY(NM_KILLFOCUS,          NMHDR),
    SPNFY(NM_CUSTOMDRAW,         NMCUSTOMDRAW),
    SPNFY(NM_HOVER,              NMHDR),
    SPNFY(NM_NCHITTEST,          NMMOUSE),
    SPNFY(NM_KEYDOWN,            NMKEY),
    SPNFY(NM_RELEASEDCAPTURE,    NMHDR),
    SPNFY(NM_SETCURSOR,          NMMOUSE),
    SPNFY(NM_CHAR,               NMCHAR),
    SPNFY(NM_TOOLTIPSCREATED,    NMTOOLTIPSCREATED),
    /* Listview       0U-100U  to  0U-199U  */
    SPNFY(LVN_ITEMCHANGING,      NMLISTVIEW),
    SPNFY(LVN_ITEMCHANGED,       NMLISTVIEW),
    SPNFY(LVN_INSERTITEM,        NMLISTVIEW),
    SPNFY(LVN_DELETEITEM,        NMLISTVIEW),
    SPNFY(LVN_DELETEALLITEMS,    NMLISTVIEW),
    SPNFY(LVN_BEGINLABELEDITA,   NMLVDISPINFOA),
    SPNFY(LVN_ENDLABELEDITA,     NMLVDISPINFOA),
    SPNFY(LVN_COLUMNCLICK,       NMLISTVIEW),
    SPNFY(LVN_BEGINDRAG,         NMLISTVIEW),
    SPNFY(LVN_BEGINRDRAG,        NMLISTVIEW),
    SPNFY(LVN_ODCACHEHINT,       NMLVCACHEHINT),
    SPNFY(LVN_ITEMACTIVATE,      NMITEMACTIVATE),
    SPNFY(LVN_ODSTATECHANGED,    NMLVODSTATECHANGE),
    SPNFY(LVN_HOTTRACK,          NMLISTVIEW),
    SPNFY(LVN_GETDISPINFOA,      NMLVDISPINFOA),
    SPNFY(LVN_SETDISPINFOA,      NMLVDISPINFOA),
    SPNFY(LVN_ODFINDITEMA,       NMLVFINDITEMA),
    SPNFY(LVN_KEYDOWN,           NMLVKEYDOWN),
    SPNFY(LVN_MARQUEEBEGIN,      NMLISTVIEW),
    SPNFY(LVN_GETINFOTIPA,       NMLVGETINFOTIPA),
    SPNFY(LVN_GETINFOTIPW,       NMLVGETINFOTIPW),
    SPNFY(LVN_BEGINLABELEDITW,   NMLVDISPINFOW),
    SPNFY(LVN_ENDLABELEDITW,     NMLVDISPINFOW),
    SPNFY(LVN_GETDISPINFOW,      NMLVDISPINFOW),
    SPNFY(LVN_SETDISPINFOW,      NMLVDISPINFOW),
    SPNFY(LVN_ODFINDITEMW,       NMLVFINDITEMW),
    /* PropertySheet  0U-200U  to  0U-299U  */
    SPNFY(PSN_SETACTIVE,         PSHNOTIFY),
    SPNFY(PSN_KILLACTIVE,        PSHNOTIFY),
    SPNFY(PSN_APPLY,             PSHNOTIFY),
    SPNFY(PSN_RESET,             PSHNOTIFY),
    SPNFY(PSN_HELP,              PSHNOTIFY),
    SPNFY(PSN_WIZBACK,           PSHNOTIFY),
    SPNFY(PSN_WIZNEXT,           PSHNOTIFY),
    SPNFY(PSN_WIZFINISH,         PSHNOTIFY),
    SPNFY(PSN_QUERYCANCEL,       PSHNOTIFY),
    SPNFY(PSN_GETOBJECT,         NMOBJECTNOTIFY),
    SPNFY(PSN_TRANSLATEACCELERATOR, PSHNOTIFY),
    SPNFY(PSN_QUERYINITIALFOCUS, PSHNOTIFY),
    /* Header         0U-300U  to  0U-399U  */
    SPNFY(HDN_ITEMCHANGINGA,     NMHEADERA),
    SPNFY(HDN_ITEMCHANGEDA,      NMHEADERA),
    SPNFY(HDN_ITEMCLICKA,        NMHEADERA),
    SPNFY(HDN_ITEMDBLCLICKA,     NMHEADERA),
    SPNFY(HDN_DIVIDERDBLCLICKA,  NMHEADERA),
    SPNFY(HDN_BEGINTRACKA,       NMHEADERA),
    SPNFY(HDN_ENDTRACKA,         NMHEADERA),
    SPNFY(HDN_TRACKA,            NMHEADERA),
    SPNFY(HDN_GETDISPINFOA,      NMHEADERA),
    SPNFY(HDN_BEGINDRAG,         NMHDR),
    SPNFY(HDN_ENDDRAG,           NMHDR),
    SPNFY(HDN_ITEMCHANGINGW,     NMHDR),
    SPNFY(HDN_ITEMCHANGEDW,      NMHDR),
    SPNFY(HDN_ITEMCLICKW,        NMHDR),
    SPNFY(HDN_ITEMDBLCLICKW,     NMHDR),
    SPNFY(HDN_DIVIDERDBLCLICKW,  NMHDR),
    SPNFY(HDN_BEGINTRACKW,       NMHDR),
    SPNFY(HDN_ENDTRACKW,         NMHDR),
    SPNFY(HDN_TRACKW,            NMHDR),
    SPNFY(HDN_GETDISPINFOW,      NMHDR),
    /* Treeview       0U-400U  to  0U-499U  */
    SPNFY(TVN_SELCHANGINGA,      NMTREEVIEWA),
    SPNFY(TVN_SELCHANGEDA,       NMTREEVIEWA),
    SPNFY(TVN_GETDISPINFOA,      NMTVDISPINFOA),
    SPNFY(TVN_SETDISPINFOA,      NMTVDISPINFOA),
    SPNFY(TVN_ITEMEXPANDINGA,    NMTREEVIEWA),
    SPNFY(TVN_ITEMEXPANDEDA,     NMTREEVIEWA),
    SPNFY(TVN_BEGINDRAGA,        NMTREEVIEWA),
    SPNFY(TVN_BEGINRDRAGA,       NMTREEVIEWA),
    SPNFY(TVN_DELETEITEMA,       NMTREEVIEWA),
    SPNFY(TVN_BEGINLABELEDITA,   NMTVDISPINFOA),
    SPNFY(TVN_ENDLABELEDITA,     NMTVDISPINFOA),
    SPNFY(TVN_KEYDOWN,           NMTVKEYDOWN),
    SPNFY(TVN_SELCHANGINGW,      NMTREEVIEWW),
    SPNFY(TVN_SELCHANGEDW,       NMTREEVIEWW),
    SPNFY(TVN_GETDISPINFOW,      NMTVDISPINFOW),
    SPNFY(TVN_SETDISPINFOW,      NMTVDISPINFOW),
    SPNFY(TVN_ITEMEXPANDINGW,    NMTREEVIEWW),
    SPNFY(TVN_ITEMEXPANDEDW,     NMTREEVIEWW),
    SPNFY(TVN_BEGINDRAGW,        NMTREEVIEWW),
    SPNFY(TVN_BEGINRDRAGW,       NMTREEVIEWW),
    SPNFY(TVN_DELETEITEMW,       NMTREEVIEWW),
    SPNFY(TVN_BEGINLABELEDITW,   NMTVDISPINFOW),
    SPNFY(TVN_ENDLABELEDITW,     NMTVDISPINFOW),
    /* Tooltips       0U-520U  to  0U-549U  */
    SPNFY(TTN_GETDISPINFOA,      NMHDR),
    SPNFY(TTN_SHOW,              NMHDR),
    SPNFY(TTN_POP,               NMHDR),
    SPNFY(TTN_GETDISPINFOW,      NMHDR),
    /* Tab            0U-550U  to  0U-580U  */
    SPNFY(TCN_KEYDOWN,           NMHDR),
    SPNFY(TCN_SELCHANGE,         NMHDR),
    SPNFY(TCN_SELCHANGING,       NMHDR),
    SPNFY(TCN_GETOBJECT,         NMHDR),
    /* Common Dialog  0U-601U  to  0U-699U  */
    SPNFY(CDN_INITDONE,          OFNOTIFYA),
    SPNFY(CDN_SELCHANGE,         OFNOTIFYA),
    SPNFY(CDN_FOLDERCHANGE,      OFNOTIFYA),
    SPNFY(CDN_SHAREVIOLATION,    OFNOTIFYA),
    SPNFY(CDN_HELP,              OFNOTIFYA),
    SPNFY(CDN_FILEOK,            OFNOTIFYA),
    SPNFY(CDN_TYPECHANGE,        OFNOTIFYA),
    /* Toolbar        0U-700U  to  0U-720U  */
    SPNFY(TBN_GETBUTTONINFOA,    NMTOOLBARA),
    SPNFY(TBN_BEGINDRAG,         NMTOOLBARA),
    SPNFY(TBN_ENDDRAG,           NMTOOLBARA),
    SPNFY(TBN_BEGINADJUST,       NMHDR),
    SPNFY(TBN_ENDADJUST,         NMHDR),
    SPNFY(TBN_RESET,             NMHDR),
    SPNFY(TBN_QUERYINSERT,       NMTOOLBARA),
    SPNFY(TBN_QUERYDELETE,       NMTOOLBARA),
    SPNFY(TBN_TOOLBARCHANGE,     NMHDR),
    SPNFY(TBN_CUSTHELP,          NMHDR),
    SPNFY(TBN_DROPDOWN,          NMTOOLBARA),
    SPNFY(TBN_GETOBJECT,         NMOBJECTNOTIFY),
    SPNFY(TBN_HOTITEMCHANGE,     NMTBHOTITEM),
    SPNFY(TBN_DRAGOUT,           NMTOOLBARA),
    SPNFY(TBN_DELETINGBUTTON,    NMTOOLBARA),
    SPNFY(TBN_GETDISPINFOA,      NMTBDISPINFOA),
    SPNFY(TBN_GETDISPINFOW,      NMTBDISPINFOW),
    SPNFY(TBN_GETINFOTIPA,       NMTBGETINFOTIPA),
    SPNFY(TBN_GETINFOTIPW,       NMTBGETINFOTIPW),
    SPNFY(TBN_GETBUTTONINFOW,    NMTOOLBARW),
    /* Up/Down        0U-721U  to  0U-740U  */
    SPNFY(UDN_DELTAPOS,          NM_UPDOWN),
    /* Month Calendar 0U-750U  to  0U-759U  */
    /* ******************* WARNING ***************************** */
    /* The following appear backwards but needs to be this way.  */
    /* The reason is that MS (and us) define the MCNs as         */
    /*         MCN_FIRST + n                                     */
    /* instead of the way ALL other notifications are            */
    /*         TBN_FIRST - n                                     */
    /* The only place that this is important is in this list     */
    /*                                                           */
    /* Also since the same error was made with the DTN_ items,   */
    /* they overlay the MCN_ and need to be inserted in the      */
    /* other section of the table so that it is in order for     */
    /* the binary search.                                        */
    /*                                                           */
    /* Thank you MS for your obvious quality control!!           */
    /* ******************* WARNING ***************************** */
    /* Date/Time      0U-760U  to  0U-799U  */
    /* SPNFY(MCN_SELECT,            NMHDR), */
    /* SPNFY(MCN_GETDAYSTATE,       NMHDR), */
    /* SPNFY(MCN_SELCHANGE,         NMHDR), */
    /* ******************* WARNING ***************************** */
    /* The following appear backwards but needs to be this way.  */
    /* The reason is that MS (and us) define the MCNs as         */
    /*         DTN_FIRST + n                                     */
    /* instead of the way ALL other notifications are            */
    /*         TBN_FIRST - n                                     */
    /* The only place that this is important is in this list     */
    /* ******************* WARNING ***************************** */
    SPNFY(DTN_FORMATQUERYW,      NMHDR),
    SPNFY(DTN_FORMATW,           NMHDR),
    SPNFY(DTN_WMKEYDOWNW,        NMHDR),
    SPNFY(DTN_USERSTRINGW,       NMHDR),
    SPNFY(MCN_SELECT,            NMHDR),
    SPNFY(MCN_GETDAYSTATE,       NMHDR),
    SPNFY(MCN_SELCHANGE,         NMHDR),
    SPNFY(DTN_CLOSEUP,           NMHDR),
    SPNFY(DTN_DROPDOWN,          NMHDR),
    SPNFY(DTN_FORMATQUERYA,      NMHDR),
    SPNFY(DTN_FORMATA,           NMHDR),
    SPNFY(DTN_WMKEYDOWNA,        NMHDR),
    SPNFY(DTN_USERSTRINGA,       NMHDR),
    SPNFY(DTN_DATETIMECHANGE,    NMHDR),
    /* ComboBoxEx     0U-800U  to  0U-830U  */
    SPNFY(CBEN_GETDISPINFOA,     NMCOMBOBOXEXA),
    SPNFY(CBEN_INSERTITEM,       NMCOMBOBOXEXA),
    SPNFY(CBEN_DELETEITEM,       NMCOMBOBOXEXA),
    SPNFY(CBEN_BEGINEDIT,        NMHDR),
    SPNFY(CBEN_ENDEDITA,         NMCBEENDEDITA),
    SPNFY(CBEN_ENDEDITW,         NMCBEENDEDITW),
    SPNFY(CBEN_GETDISPINFOW,     NMCOMBOBOXEXW),
    SPNFY(CBEN_DRAGBEGINA,       NMCBEDRAGBEGINA),
    SPNFY(CBEN_DRAGBEGINW,       NMCBEDRAGBEGINW),
    /* Rebar          0U-831U  to  0U-859U  */
    SPNFY(RBN_HEIGHTCHANGE,      NMHDR),
    SPNFY(RBN_GETOBJECT,         NMOBJECTNOTIFY),
    SPNFY(RBN_LAYOUTCHANGED,     NMHDR),
    SPNFY(RBN_AUTOSIZE,          NMRBAUTOSIZE),
    SPNFY(RBN_BEGINDRAG,         NMREBAR),
    SPNFY(RBN_ENDDRAG,           NMREBAR),
    SPNFY(RBN_DELETINGBAND,      NMREBAR),
    SPNFY(RBN_DELETEDBAND,       NMREBAR),
    SPNFY(RBN_CHILDSIZE,         NMREBARCHILDSIZE),
    /* IP Adderss     0U-860U  to  0U-879U  */
    SPNFY(IPN_FIELDCHANGED,      NMHDR),
    /* Status bar     0U-880U  to  0U-899U  */
    SPNFY(SBN_SIMPLEMODECHANGE,  NMHDR),
    /* Pager          0U-900U  to  0U-950U  */
    SPNFY(PGN_SCROLL,            NMPGSCROLL),
    SPNFY(PGN_CALCSIZE,          NMPGCALCSIZE),
    {0,0,0}};
static const SPY_NOTIFY *end_spnfy_array;     /* ptr to last good entry in array */
#undef SPNFY

static BOOL SPY_Exclude[SPY_MAX_MSGNUM+1];
static BOOL SPY_ExcludeDWP = 0;

#define SPY_EXCLUDE(msg) \
    (SPY_Exclude[(msg) > SPY_MAX_MSGNUM ? SPY_MAX_MSGNUM : (msg)])


typedef struct
{
    UINT       msgnum;           /* message number */
    HWND       msg_hwnd;         /* window handle for message          */
    WPARAM     wParam;           /* message parameter                  */
    LPARAM     lParam;           /* message parameter                  */
    INT        data_len;         /* length of data to dump             */
    char       msg_name[60];     /* message name (see SPY_GetMsgName)  */
    WCHAR      wnd_class[60];    /* window class name (full)           */
    WCHAR      wnd_name[16];     /* window name for message            */
} SPY_INSTANCE;

static int indent_tls_index;

/***********************************************************************
 *           get_indent_level
 */
__inline static INT_PTR get_indent_level(void)
{
    return (INT_PTR)TlsGetValue( indent_tls_index );
}


/***********************************************************************
 *           set_indent_level
 */
__inline static void set_indent_level( INT_PTR level )
{
    TlsSetValue( indent_tls_index, (void *)level );
}


/***********************************************************************
 *           SPY_GetMsgInternal
 */
static const char *SPY_GetMsgInternal( UINT msg )
{
    if (msg <= SPY_MAX_MSGNUM)
        return MessageTypeNames[msg];

    if (msg >= LVM_FIRST && msg <= LVM_FIRST + SPY_MAX_LVMMSGNUM)
        return LVMMessageTypeNames[msg-LVM_FIRST];

    if (msg >= TV_FIRST && msg <= TV_FIRST + SPY_MAX_TVMSGNUM)
        return TVMessageTypeNames[msg-TV_FIRST];

    if (msg >= HDM_FIRST && msg <= HDM_FIRST + SPY_MAX_HDMMSGNUM)
        return HDMMessageTypeNames[msg-HDM_FIRST];

    if (msg >= TCM_FIRST && msg <= TCM_FIRST + SPY_MAX_TCMMSGNUM)
        return TCMMessageTypeNames[msg-TCM_FIRST];

    if (msg >= PGM_FIRST && msg <= PGM_FIRST + SPY_MAX_PGMMSGNUM)
        return PGMMessageTypeNames[msg-PGM_FIRST];

    if (msg >= CCM_FIRST && msg <= CCM_FIRST + SPY_MAX_CCMMSGNUM)
        return CCMMessageTypeNames[msg-CCM_FIRST];
#ifndef __REACTOS__
    if (msg >= WM_WINE_DESTROYWINDOW && msg <= WM_WINE_DESTROYWINDOW + SPY_MAX_WINEMSGNUM)
        return WINEMessageTypeNames[msg-WM_WINE_DESTROYWINDOW];
#endif
    return NULL;
}

/***********************************************************************
 *           SPY_Bsearch_Msg
 */
static const USER_MSG *SPY_Bsearch_Msg( const USER_MSG *first, const USER_MSG *last, UINT code)
{
    INT count;
    const USER_MSG *test;

    while (last >= first) {
        count = 1 + last - first;
        if (count < 3) {
#if DEBUG_SPY
            TRACE("code=%d, f-value=%d, f-name=%s, l-value=%d, l-name=%s, l-len=%d,\n",
               code, first->value, first->name, last->value, last->name, last->len);
#endif
            if (first->value == code) return first;
            if (last->value == code) return last;
            return NULL;
        }
        count = count / 2;
        test = first + count;
#if DEBUG_SPY
        TRACE("first=%p, last=%p, test=%p, t-value=%d, code=%d, count=%d\n",
           first, last, test, test->value, code, count);
#endif
        if (test->value == code) return test;
        if (test->value > code)
            last = test - 1;
        else
            first = test + 1;
    }
    return NULL;
}

/***********************************************************************
 *           SPY_GetClassName
 *
 *  Sets the value of "wnd_class" member of the instance structure.
 */
static void SPY_GetClassName( SPY_INSTANCE *sp_e )
{
    DWORD save_error;

    /* save and restore error code over the next call */
    save_error = GetLastError();
    /* special code to detect a property sheet dialog   */
    if ((GetClassLongW(sp_e->msg_hwnd, GCW_ATOM) == (LONG)WC_DIALOG) &&
        (GetPropW(sp_e->msg_hwnd, PropSheetInfoStr))) {
        strcpyW(sp_e->wnd_class, WC_PROPSHEETW);
    }
    else {
        GetClassNameW(sp_e->msg_hwnd, sp_e->wnd_class, sizeof(sp_e->wnd_class)/sizeof(WCHAR));
    }
    SetLastError(save_error);
}

/***********************************************************************
 *           SPY_GetMsgStuff
 *
 *  Get message name and other information for dumping
 */
static void SPY_GetMsgStuff( SPY_INSTANCE *sp_e )
{
    const USER_MSG *p;
    const char *msg_name = SPY_GetMsgInternal( sp_e->msgnum );

    sp_e->data_len = 0;
    if (!msg_name)
    {
        INT i = 0;

        if (sp_e->msgnum >= 0xc000)
        {
            if (GlobalGetAtomNameA( sp_e->msgnum, sp_e->msg_name+1, sizeof(sp_e->msg_name)-2 ))
            {
                sp_e->msg_name[0] = '\"';
                strcat( sp_e->msg_name, "\"" );
                return;
            }
        }
        if (!sp_e->wnd_class[0]) SPY_GetClassName(sp_e);

#if DEBUG_SPY
        TRACE("looking class %s\n", debugstr_w(sp_e->wnd_class));
#endif

        while (cc_array[i].classname &&
               strcmpiW(cc_array[i].classname, sp_e->wnd_class) != 0) i++;

        if (cc_array[i].classname)
        {
#if DEBUG_SPY
            TRACE("process class %s, first %p, last %p\n",
                  debugstr_w(cc_array[i].classname), cc_array[i].classmsg,
                  cc_array[i].lastmsg);
#endif
            p = SPY_Bsearch_Msg (cc_array[i].classmsg, cc_array[i].lastmsg,
                                 sp_e->msgnum);
            if (p) {
                lstrcpynA (sp_e->msg_name, p->name, sizeof(sp_e->msg_name));
                sp_e->data_len = p->len;
                return;
            }
        }
        if (sp_e->msgnum >= WM_USER && sp_e->msgnum <= WM_APP)
            sprintf( sp_e->msg_name, "WM_USER+%d", sp_e->msgnum - WM_USER );
        else
            sprintf( sp_e->msg_name, "%04x", sp_e->msgnum );
    }
    else
    {
        lstrcpynA(sp_e->msg_name, msg_name, sizeof(sp_e->msg_name));
    }
}

/***********************************************************************
 *           SPY_GetWndName
 *
 *  Sets the value of "wnd_name" and "wnd_class" members of the
 *  instance structure.
 *
 */
static void SPY_GetWndName( SPY_INSTANCE *sp_e )
{
    INT len;

    SPY_GetClassName( sp_e );

    len = InternalGetWindowText(sp_e->msg_hwnd, sp_e->wnd_name, sizeof(sp_e->wnd_name)/sizeof(WCHAR));
    if(!len) /* get class name */
    {
        LPWSTR dst = sp_e->wnd_name;
        LPWSTR src = sp_e->wnd_class;
        int n = sizeof(sp_e->wnd_name)/sizeof(WCHAR) - 3;
        *dst++ = '{';
        while ((n-- > 0) && *src) *dst++ = *src++;
        *dst++ = '}';
        *dst = 0;
    }
}

/***********************************************************************
 *           SPY_GetMsgName
 *
 *  ****  External function  ****
 *
 *  Get message name
 */
const char *SPY_GetMsgName( UINT msg, HWND hWnd )
{
    SPY_INSTANCE ext_sp_e;

    ext_sp_e.msgnum = msg;
    ext_sp_e.msg_hwnd   = hWnd;
    ext_sp_e.lParam = 0;
    ext_sp_e.wParam = 0;
    ext_sp_e.wnd_class[0] = 0;
    SPY_GetMsgStuff(&ext_sp_e);
    return wine_dbg_sprintf("%s", ext_sp_e.msg_name);
}

/***********************************************************************
 *           SPY_GetVKeyName
 */
const char *SPY_GetVKeyName(WPARAM wParam)
{
    const char *vk_key_name;

    if(wParam <= SPY_MAX_VKKEYSNUM && VK_KeyNames[wParam])
        vk_key_name = VK_KeyNames[wParam];
    else
        vk_key_name = "VK_???";

    return vk_key_name;
}

/***********************************************************************
 *           SPY_Bsearch_Notify
 */
static const SPY_NOTIFY *SPY_Bsearch_Notify( const SPY_NOTIFY *first, const SPY_NOTIFY *last, UINT code)
{
    INT count;
    const SPY_NOTIFY *test;

    while (last >= first) {
        count = 1 + last - first;
        if (count < 3) {
#if DEBUG_SPY
            TRACE("code=%d, f-value=%d, f-name=%s, l-value=%d, l-name=%s, l-len=%d,\n",
               code, first->value, first->name, last->value, last->name, last->len);
#endif
            if (first->value == code) return first;
            if (last->value == code) return last;
            return NULL;
        }
        count = count / 2;
        test = first + count;
#if DEBUG_SPY
        TRACE("first=%p, last=%p, test=%p, t-value=%d, code=%d, count=%d\n",
           first, last, test, test->value, code, count);
#endif
        if (test->value == code) return test;
        if (test->value < code)
            last = test - 1;
        else
            first = test + 1;
    }
    return NULL;
}

/***********************************************************************
 *           SPY_DumpMem
 */
static void SPY_DumpMem (LPCSTR header, const UINT *q, INT len)
{
    int i;

    for(i=0; i<len-12; i+=16) {
        TRACE("%s [%04x] %08x %08x %08x %08x\n",
              header, i, *q, *(q+1), *(q+2), *(q+3));
        q += 4;
    }
    switch ((len - i + 3) & (~3)) {
    case 16:
        TRACE("%s [%04x] %08x %08x %08x %08x\n",
              header, i, *q, *(q+1), *(q+2), *(q+3));
        break;
    case 12:
        TRACE("%s [%04x] %08x %08x %08x\n",
              header, i, *q, *(q+1), *(q+2));
        break;
    case 8:
        TRACE("%s [%04x] %08x %08x\n",
              header, i, *q, *(q+1));
        break;
    case 4:
        TRACE("%s [%04x] %08x\n",
              header, i, *q);
        break;
    default:
        break;
    }
}

/***********************************************************************
 *           SPY_DumpStructure
 */
static void SPY_DumpStructure(const SPY_INSTANCE *sp_e, BOOL enter)
{
    switch (sp_e->msgnum)
        {
        case LVM_INSERTITEMW:
        case LVM_INSERTITEMA:
        case LVM_SETITEMW:
        case LVM_SETITEMA:
            if (!enter) break;
            /* fall through */
        case LVM_GETITEMW:
        case LVM_GETITEMA:
            {
                LPLVITEMA item = (LPLVITEMA) sp_e->lParam;
                if (item) {
                    SPY_DumpMem ("LVITEM", (UINT*)item, sizeof(LVITEMA));
                }
                break;
            }
        case TCM_INSERTITEMW:
        case TCM_INSERTITEMA:
        case TCM_SETITEMW:
        case TCM_SETITEMA:
            if (!enter) break;
            /* fall through */
        case TCM_GETITEMW:
        case TCM_GETITEMA:
            {
                TCITEMA *item = (TCITEMA *) sp_e->lParam;
                if (item) {
                    SPY_DumpMem ("TCITEM", (UINT*)item, sizeof(TCITEMA));
                }
                break;
            }
        case TCM_ADJUSTRECT:
        case LVM_GETITEMRECT:
        case LVM_GETSUBITEMRECT:
            {
                LPRECT rc = (LPRECT) sp_e->lParam;
                if (rc) {
                    TRACE("lParam rect (%ld,%ld)-(%ld,%ld)\n",
                          rc->left, rc->top, rc->right, rc->bottom);
                }
                break;
            }
        case LVM_SETITEMPOSITION32:
            if (!enter) break;
            /* fall through */
        case LVM_GETITEMPOSITION:
        case LVM_GETORIGIN:
            {
                LPPOINT point = (LPPOINT) sp_e->lParam;
                if (point) {
                    TRACE("lParam point x=%ld, y=%ld\n", point->x, point->y);
                }
                break;
            }
        case SBM_SETRANGE:
            if (!enter && (sp_e->msgnum == SBM_SETRANGE)) break;
            TRACE("min=%d max=%d\n", (INT)sp_e->wParam, (INT)sp_e->lParam);
            break;
        case SBM_GETRANGE:
            if ((enter && (sp_e->msgnum == SBM_GETRANGE)) ||
                (!enter && (sp_e->msgnum == SBM_SETRANGE))) break;
            {
                LPINT ptmin = (LPINT) sp_e->wParam;
                LPINT ptmax = (LPINT) sp_e->lParam;
                if (ptmin && ptmax)
                    TRACE("min=%d max=%d\n", *ptmin, *ptmax);
                else if (ptmin)
                    TRACE("min=%d max=n/a\n", *ptmin);
                else if (ptmax)
                    TRACE("min=n/a max=%d\n", *ptmax);
                break;
            }
        case EM_EXSETSEL:
            if (enter && sp_e->lParam)
            {
                CHARRANGE *cr = (CHARRANGE *) sp_e->lParam;
                TRACE("CHARRANGE: cpMin=%ld cpMax=%ld\n", cr->cpMin, cr->cpMax);
            }
            break;
        case EM_SETCHARFORMAT:
            if (enter && sp_e->lParam)
            {
                CHARFORMATW *cf = (CHARFORMATW *) sp_e->lParam;
                TRACE("CHARFORMAT: dwMask=0x%08lx dwEffects=", cf->dwMask);
                if ((cf->dwMask & CFM_BOLD) && (cf->dwEffects & CFE_BOLD))
                    TRACE(" CFE_BOLD");
                if ((cf->dwMask & CFM_COLOR) && (cf->dwEffects & CFE_AUTOCOLOR))
                    TRACE(" CFE_AUTOCOLOR");
                if ((cf->dwMask & CFM_ITALIC) && (cf->dwEffects & CFE_ITALIC))
                    TRACE(" CFE_ITALIC");
                if ((cf->dwMask & CFM_PROTECTED) && (cf->dwEffects & CFE_PROTECTED))
                    TRACE(" CFE_PROTECTED");
                if ((cf->dwMask & CFM_STRIKEOUT) && (cf->dwEffects & CFE_STRIKEOUT))
                    TRACE(" CFE_STRIKEOUT");
                if ((cf->dwMask & CFM_UNDERLINE) && (cf->dwEffects & CFE_UNDERLINE))
                    TRACE(" CFE_UNDERLINE");
                TRACE("\n");
                if (cf->dwMask & CFM_SIZE)
                    TRACE("yHeight=%ld\n", cf->yHeight);
                if (cf->dwMask & CFM_OFFSET)
                    TRACE("yOffset=%ld\n", cf->yOffset);
                if ((cf->dwMask & CFM_COLOR) && !(cf->dwEffects & CFE_AUTOCOLOR))
                    TRACE("crTextColor=%lx\n", cf->crTextColor);
                TRACE("bCharSet=%x bPitchAndFamily=%x\n", cf->bCharSet, cf->bPitchAndFamily);
                /* FIXME: we should try to be a bit more intelligent about
                 * whether this is in ANSI or Unicode (it could be either) */
                if (cf->dwMask & CFM_FACE)
                    TRACE("szFaceName=%s\n", debugstr_wn(cf->szFaceName, LF_FACESIZE));
                /* FIXME: handle CHARFORMAT2 too */
            }
            break;
        case WM_DRAWITEM:
            if (!enter) break;
            {
                DRAWITEMSTRUCT *lpdis = (DRAWITEMSTRUCT*) sp_e->lParam;
                TRACE("DRAWITEMSTRUCT: CtlType=0x%08x CtlID=0x%08x\n",
                      lpdis->CtlType, lpdis->CtlID);
                TRACE("itemID=0x%08x itemAction=0x%08x itemState=0x%08x\n",
                      lpdis->itemID, lpdis->itemAction, lpdis->itemState);
                TRACE("hWnd=%p hDC=%p (%ld,%ld)-(%ld,%ld) itemData=0x%08lx\n",
                      lpdis->hwndItem, lpdis->hDC, lpdis->rcItem.left,
                      lpdis->rcItem.top, lpdis->rcItem.right,
                      lpdis->rcItem.bottom, lpdis->itemData);
            }
            break;
        case WM_MEASUREITEM:
            {
                MEASUREITEMSTRUCT *lpmis = (MEASUREITEMSTRUCT*) sp_e->lParam;
                TRACE("MEASUREITEMSTRUCT: CtlType=0x%08x CtlID=0x%08x\n",
                      lpmis->CtlType, lpmis->CtlID);
                TRACE("itemID=0x%08x itemWidth=0x%08x itemHeight=0x%08x\n",
                      lpmis->itemID, lpmis->itemWidth, lpmis->itemHeight);
                TRACE("itemData=0x%08lx\n", lpmis->itemData);
            }
            break;
        case WM_SIZE:
            if (!enter) break;
            TRACE("cx=%d cy=%d\n", LOWORD(sp_e->lParam), HIWORD(sp_e->lParam));
            break;
        case WM_WINDOWPOSCHANGED:
            if (!enter) break;
        case WM_WINDOWPOSCHANGING:
            {
                WINDOWPOS *lpwp = (WINDOWPOS *)sp_e->lParam;
                TRACE("WINDOWPOS hwnd=%p, after=%p, at (%d,%d) w=%d h=%d, flags=0x%08x\n",
                      lpwp->hwnd, lpwp->hwndInsertAfter, lpwp->x, lpwp->y,
                      lpwp->cx, lpwp->cy, lpwp->flags);
            }
            break;
        case WM_STYLECHANGED:
            if (!enter) break;
        case WM_STYLECHANGING:
            {
                LPSTYLESTRUCT ss = (LPSTYLESTRUCT) sp_e->lParam;
                TRACE("STYLESTRUCT: StyleOld=0x%08lx, StyleNew=0x%08lx\n",
                      ss->styleOld, ss->styleNew);
            }
            break;
        case WM_NCCALCSIZE:
            {
                RECT *rc = (RECT *)sp_e->lParam;
                TRACE("Rect (%ld,%ld)-(%ld,%ld)\n",
                      rc->left, rc->top, rc->right, rc->bottom);
            }
            break;
        case WM_NOTIFY:
            /* if (!enter) break; */
            {
                NMHDR * pnmh = (NMHDR*) sp_e->lParam;
                UINT *q, dumplen;
                const SPY_NOTIFY *p;
                WCHAR from_class[60];
                DWORD save_error;

                p = SPY_Bsearch_Notify (&spnfy_array[0], end_spnfy_array,
                                        pnmh->code);
                if (p) {
                    TRACE("NMHDR hwndFrom=%p idFrom=0x%08lx code=%s<0x%08x>, extra=0x%x\n",
                          pnmh->hwndFrom, pnmh->idFrom, p->name, pnmh->code, p->len);
                    dumplen = p->len;

                    /* for CUSTOMDRAW, dump all the data for TOOLBARs */
                    if (pnmh->code == NM_CUSTOMDRAW) {
                        /* save and restore error code over the next call */
                        save_error = GetLastError();
                        GetClassNameW(pnmh->hwndFrom, from_class,
                                      sizeof(from_class)/sizeof(WCHAR));
                        SetLastError(save_error);
                        if (strcmpW(TOOLBARCLASSNAMEW, from_class) == 0)
                            dumplen = sizeof(NMTBCUSTOMDRAW)-sizeof(NMHDR);
                    } else if ((pnmh->code >= HDN_ITEMCHANGINGA) && (pnmh->code <= HDN_ENDDRAG)) {
                        dumplen = sizeof(NMHEADERA)-sizeof(NMHDR);
                    }
                    if (dumplen > 0) {
                        q = (UINT *)(pnmh + 1);
                        SPY_DumpMem ("NM extra", q, (INT)dumplen);
                    }
                }
                else
                    TRACE("NMHDR hwndFrom=%p idFrom=0x%08lx code=0x%08x\n",
                          pnmh->hwndFrom, pnmh->idFrom, pnmh->code);
            }
        default:
            if (sp_e->data_len > 0)
                SPY_DumpMem ("MSG lParam", (UINT *)sp_e->lParam, sp_e->data_len);
            break;
        }

}
/***********************************************************************
 *           SPY_EnterMessage
 */
void SPY_EnterMessage( INT iFlag, HWND hWnd, UINT msg,
                       WPARAM wParam, LPARAM lParam )
{
    SPY_INSTANCE sp_e;
    int indent;

    if (!TRACE_ON(message) || SPY_EXCLUDE(msg)) return;

    sp_e.msgnum = msg;
    sp_e.msg_hwnd = hWnd;
    sp_e.lParam = lParam;
    sp_e.wParam = wParam;
    SPY_GetWndName(&sp_e);
    SPY_GetMsgStuff(&sp_e);
    indent = get_indent_level();

    /* each SPY_SENDMESSAGE must be complemented by call to SPY_ExitMessage */
    switch(iFlag)
    {
#ifndef __REACTOS__
    case SPY_DISPATCHMESSAGE16:
        TRACE("%*s(%04x) %-16s message [%04x] %s dispatched  wp=%04lx lp=%08lx\n",
              indent, "", HWND_16(hWnd),
              debugstr_w(sp_e.wnd_name), msg, sp_e.msg_name, wParam, lParam);
        break;
#endif
    case SPY_DISPATCHMESSAGE:
        TRACE("%*s(%p) %-16s message [%04x] %s dispatched  wp=%08lx lp=%08lx\n",
                        indent, "", hWnd, debugstr_w(sp_e.wnd_name), msg,
                        sp_e.msg_name, wParam, lParam);
        break;

//    case SPY_SENDMESSAGE16:
    case SPY_SENDMESSAGE:
        {
            char taskName[20];
            DWORD tid = GetWindowThreadProcessId( hWnd, NULL );

            if (tid == GetCurrentThreadId()) strcpy( taskName, "self" );
            else sprintf( taskName, "tid %04lx", GetCurrentThreadId() );
#ifndef __REACTOS__
            if (iFlag == SPY_SENDMESSAGE16)
                TRACE("%*s(%04x) %-16s message [%04x] %s sent from %s wp=%04lx lp=%08lx\n",
                      indent, "", HWND_16(hWnd), debugstr_w(sp_e.wnd_name), msg,
                      sp_e.msg_name, taskName, wParam, lParam );
            else
#endif
            {   TRACE("%*s(%p) %-16s message [%04x] %s sent from %s wp=%08lx lp=%08lx\n",
                             indent, "", hWnd, debugstr_w(sp_e.wnd_name), msg,
                             sp_e.msg_name, taskName, wParam, lParam );
                SPY_DumpStructure(&sp_e, TRUE);
            }
        }
        break;

#ifndef __REACTOS__
    case SPY_DEFWNDPROC16:
        if( SPY_ExcludeDWP ) return;
        TRACE("%*s(%04x)  DefWindowProc16: %s [%04x]  wp=%04lx lp=%08lx\n",
              indent, "", HWND_16(hWnd), sp_e.msg_name, msg, wParam, lParam );
        break;
#endif

    case SPY_DEFWNDPROC:
        if( SPY_ExcludeDWP ) return;
        TRACE("%*s(%p)  DefWindowProc32: %s [%04x]  wp=%08lx lp=%08lx\n",
                        indent, "", hWnd, sp_e.msg_name,
                        msg, wParam, lParam );
        break;
    }
    set_indent_level( indent + SPY_INDENT_UNIT );
}


/***********************************************************************
 *           SPY_ExitMessage
 */
void SPY_ExitMessage( INT iFlag, HWND hWnd, UINT msg, LRESULT lReturn,
                       WPARAM wParam, LPARAM lParam )
{
    SPY_INSTANCE sp_e;
    int indent;

    if (!TRACE_ON(message) || SPY_EXCLUDE(msg) ||
        (SPY_ExcludeDWP && (/*iFlag == SPY_RESULT_DEFWND16 || */iFlag == SPY_RESULT_DEFWND)) )
        return;

    sp_e.msgnum = msg;
    sp_e.msg_hwnd   = hWnd;
    sp_e.lParam = lParam;
    sp_e.wParam = wParam;
    SPY_GetWndName(&sp_e);
    SPY_GetMsgStuff(&sp_e);

    if ((indent = get_indent_level()))
    {
        indent -= SPY_INDENT_UNIT;
        set_indent_level( indent );
    }

    switch(iFlag)
    {
#ifndef __REACTOS__
    case SPY_RESULT_DEFWND16:
        TRACE(" %*s(%04x)  DefWindowProc16: %s [%04x] returned %08lx\n",
              indent, "", HWND_16(hWnd), sp_e.msg_name, msg, lReturn );
        break;
#endif

    case SPY_RESULT_DEFWND:
        TRACE(" %*s(%p)  DefWindowProc32: %s [%04x] returned %08lx\n",
                        indent, "", hWnd, sp_e.msg_name, msg, lReturn );
        break;

#ifndef __REACTOS__
    case SPY_RESULT_OK16:
        TRACE(" %*s(%04x) %-16s message [%04x] %s returned %08lx\n",
              indent, "", HWND_16(hWnd), debugstr_w(sp_e.wnd_name), msg,
              sp_e.msg_name, lReturn );
        break;
#endif

    case SPY_RESULT_OK:
        TRACE(" %*s(%p) %-16s message [%04x] %s returned %08lx\n",
                        indent, "", hWnd, debugstr_w(sp_e.wnd_name), msg,
                        sp_e.msg_name, lReturn );
        SPY_DumpStructure(&sp_e, FALSE);
        break;

#ifndef __REACTOS__
    case SPY_RESULT_INVALIDHWND16:
        WARN(" %*s(%04x) %-16s message [%04x] %s HAS INVALID HWND\n",
             indent, "", HWND_16(hWnd), debugstr_w(sp_e.wnd_name), msg, sp_e.msg_name );
        break;
#endif

    case SPY_RESULT_INVALIDHWND:
        WARN(" %*s(%p) %-16s message [%04x] %s HAS INVALID HWND\n",
                        indent, "", hWnd, debugstr_w(sp_e.wnd_name), msg,
                        sp_e.msg_name );
        break;
   }
}


/***********************************************************************
 *           SPY_Init
 */
int SPY_Init(void)
{
    int i, j;
    char buffer[1024];
    const SPY_NOTIFY *p;
    const USER_MSG *q;
    HKEY hkey;

    if (!TRACE_ON(message)) return TRUE;

    indent_tls_index = TlsAlloc();
    /* @@ Wine registry key: HKCU\Software\Wine\Debug */
    if(!RegOpenKeyA(HKEY_CURRENT_USER, "Software\\ReactOS\\Debug", &hkey))
    {
        DWORD type, count = sizeof(buffer);

        buffer[0] = 0;
        if (!RegQueryValueExA(hkey, "SpyInclude", 0, &type, (LPBYTE) buffer, &count) &&
            strcmp( buffer, "INCLUDEALL" ))
        {
            TRACE("Include=%s\n", buffer );
            for (i = 0; i <= SPY_MAX_MSGNUM; i++)
                SPY_Exclude[i] = (MessageTypeNames[i] && !strstr(buffer,MessageTypeNames[i]));
        }

        count = sizeof(buffer);
        buffer[0] = 0;
        if (!RegQueryValueExA(hkey, "SpyExclude", 0, &type, (LPBYTE) buffer, &count))
        {
            TRACE("Exclude=%s\n", buffer );
            if (!strcmp( buffer, "EXCLUDEALL" ))
                for (i = 0; i <= SPY_MAX_MSGNUM; i++) SPY_Exclude[i] = TRUE;
            else
                for (i = 0; i <= SPY_MAX_MSGNUM; i++)
                    SPY_Exclude[i] = (MessageTypeNames[i] && strstr(buffer,MessageTypeNames[i]));
        }

        SPY_ExcludeDWP = 0;
        count = sizeof(buffer);
        if(!RegQueryValueExA(hkey, "SpyExcludeDWP", 0, &type, (LPBYTE) buffer, &count))
            SPY_ExcludeDWP = atoi(buffer);

        RegCloseKey(hkey);
    }

    /* find last good entry in spy notify array and save addr for b-search */
    p = &spnfy_array[0];
    j = 0xffffffff;
    while (p->name) {
        if ((UINT)p->value > (UINT)j) {
            ERR("Notify message array out of order\n");
            ERR("  between values [%08x] %s and [%08x] %s\n",
                j, (p-1)->name, p->value, p->name);
            break;
        }
        j = p->value;
        p++;
    }
    p--;
    end_spnfy_array = p;

    /* find last good entry in each common control message array
     *  and save addr for b-search.
     */
    i = 0;
    while (cc_array[i].classname) {

        j = 0x0400; /* minimum entry in array */
        q = cc_array[i].classmsg;
        while(q->name) {
            if (q->value <= j) {
                ERR("Class message array out of order for class %s\n",
                    debugstr_w(cc_array[i].classname));
                ERR("  between values [%04x] %s and [%04x] %s\n",
                    j, (q-1)->name, q->value, q->name);
                break;
            }
            j = q->value;
            q++;
        }
        q--;
        cc_array[i].lastmsg = (USER_MSG *)q;

        i++;
    }

    return 1;
}
