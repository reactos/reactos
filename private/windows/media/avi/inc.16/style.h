/*****************************************************************************\
*                                                                             *
* style.h -	 Common style ID's                                            *
*									      *
* Copyright (c) 1993-1994, Microsoft Corp.	All rights reserved.	      *
*                                                                             *
\*****************************************************************************/

/* Window styles */
#define WS_TILED        0x00000000L
#define WS_ICONICPOPUP  0xc0000000L
#define WS_POPUP        0x80000000L
#define WS_CHILD        0x40000000L
#define WS_MINIMIZE     0x20000000L
#define WS_VISIBLE      0x10000000L
#define WS_DISABLED     0x08000000L
#define WS_CLIPSIBLINGS 0x04000000L
#define WS_CLIPCHILDREN 0x02000000L
#define WS_MAXIMIZE     0x01000000L

#define WS_BORDER       0x00800000L
#define WS_CAPTION      0x00c00000L
#define WS_DLGFRAME     0x00400000L
#define WS_VSCROLL      0x00200000L
#define WS_HSCROLL      0x00100000L
#define WS_SYSMENU      0x00080000L
#define WS_SIZEBOX      0x00040000L
#define WS_GROUP        0x00020000L
#define WS_TABSTOP      0x00010000L

#define WS_ICONIC       WS_MINIMIZE

/* Class styles */
#define CS_VREDRAW      0x0001
#define CS_HREDRAW      0x0002
#define CS_KEYCVTWINDOW 0x0004
#define CS_DBLCLKS      0x0008
			/* 0x0010 reserved */
#define CS_OWNDC        0x0020
#define CS_CLASSDC      0x0040
#define CS_MENUPOPUP    0x0080
#define CS_NOKEYCVT     0x0100
#define CS_SAVEBITS     0x0800

/* Shorthand for the common cases */
#define WS_TILEDWINDOW   (WS_TILED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX)
#define WS_POPUPWINDOW   (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_CHILDWINDOW   (WS_CHILD)

/* Edit control styles */
#define ES_LEFT             0x0000L
#define ES_CENTER           0x0001L
#define ES_RIGHT            0x0002L
#define ES_MULTILINE        0x0004L
#define ES_UPPERCASE        0x0008L
#define ES_LOWERCASE        0x0010L
#define ES_PASSWORD         0x0020L
#define ES_AUTOVSCROLL      0x0040L
#define ES_AUTOHSCROLL      0x0080L
#define ES_NOHIDESEL        0x0100L
#define ES_OEMCONVERT       0x0400L

/* button control styles */
#define BS_PUSHBUTTON    0L
#define BS_DEFPUSHBUTTON 1L
#define BS_CHECKBOX      2L
#define BS_AUTOCHECKBOX  3L
#define BS_RADIOBUTTON   4L
#define BS_3STATE        5L
#define BS_AUTO3STATE    6L
#define BS_GROUPBOX      7L
#define BS_USERBUTTON    8L
#define BS_AUTORADIOBUTTON 9L
#define BS_PUSHBOX       10L
#define BS_OWNERDRAW	   0x0BL
#define BS_LEFTTEXT      0x20L

/* Dialog Styles */
#define DS_ABSALIGN	    0x01L
#define DS_SYSMODAL	    0x02L
#define DS_LOCALEDIT	    0x20L   /* Edit items get Local storage. */
#define DS_SETFONT	    0x40L   /* User specified font for Dlg controls */
#define DS_MODALFRAME	    0x80L   /* Can be combined with WS_CAPTION  */
#define DS_NOIDLEMSG	    0x100L  /* WM_ENTERIDLE message will not be sent */

/* listbox style bits */
#define LBS_NOTIFY        0x0001L
#define LBS_SORT          0x0002L
#define LBS_NOREDRAW      0x0004L
#define LBS_MULTIPLESEL   0x0008L
#define LBS_OWNERDRAWFIXED    0x0010L
#define LBS_OWNERDRAWVARIABLE 0x0020L
#define LBS_HASSTRINGS        0x0040L
#define LBS_USETABSTOPS       0x0080L
#define LBS_NOINTEGRALHEIGHT  0x0100L
#define LBS_MULTICOLUMN       0x0200L
#define LBS_WANTKEYBOARDINPUT 0x0400L
#define LBS_EXTENDEDSEL	      0x0800L
#define LBS_STANDARD      (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)

/* Combo Box styles */
#define CBS_SIMPLE	      0x0001L
#define CBS_DROPDOWN	      0x0002L
#define CBS_DROPDOWNLIST      0x0003L
#define CBS_OWNERDRAWFIXED    0x0010L
#define CBS_OWNERDRAWVARIABLE 0x0020L
#define CBS_AUTOHSCROLL       0x0040L
#define CBS_SORT              0x0100L
#define CBS_HASSTRINGS        0x0200L

/* scroll bar styles */
#define SBS_HORZ                    0x0000L
#define SBS_VERT                    0x0001L
#define SBS_TOPALIGN                0x0002L
#define SBS_LEFTALIGN               0x0002L
#define SBS_BOTTOMALIGN             0x0004L
#define SBS_RIGHTALIGN              0x0004L
#define SBS_SIZEBOXTOPLEFTALIGN     0x0002L
#define SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L
#define SBS_SIZEBOX                 0x0008L

/* Conventional dialog box and message box command IDs */
#define IDOK         1
#define IDCANCEL     2
#define IDABORT      3
#define IDRETRY      4
#define IDIGNORE     5
#define IDYES        6
#define IDNO         7

/* Static control constants */
#define SS_LEFT       0L
#define SS_CENTER     1L
#define SS_RIGHT      2L
#define SS_ICON       3L
#define SS_BLACKRECT  4L
#define SS_GRAYRECT   5L
#define SS_WHITERECT  6L
#define SS_BLACKFRAME 7L
#define SS_GRAYFRAME  8L
#define SS_WHITEFRAME 9L
#define SS_USERITEM   10L

/* Virtual Keys, Standard Set */

#define VK_LBUTTON  0x01
#define VK_RBUTTON  0x02
#define VK_CANCEL   0x03
#define VK_MBUTTON  0x04    /* NOT contiguous with L & RBUTTON */
#define VK_BACK     0x08
#define VK_TAB      0x09
#define VK_CLEAR    0x0c
#define VK_RETURN   0x0d
#define VK_SHIFT    0x10
#define VK_CONTROL  0x11
#define VK_MENU     0x12
#define VK_PAUSE    0x13
#define VK_CAPITAL  0x14
#define VK_ESCAPE   0x1b
#define VK_SPACE    0x20

#define VK_PRIOR    0x21
#define VK_NEXT     0x22
#define VK_END      0x23
#define VK_HOME     0x24
#define VK_LEFT     0x25
#define VK_UP       0x26
#define VK_RIGHT    0x27
#define VK_DOWN     0x28

/* VK_A thru VK_Z are the same as their ASCII equivalents: 'A' thru 'Z' */
/* VK_0 thru VK_9 are the same as their ASCII equivalents: '0' thru '0' */

#define VK_SELECT   0x29
#define VK_PRINT    0x2a
#define VK_EXECUTE  0x2b
#define VK_SNAPSHOT 0x2c
#define VK_INSERT   0x2d
#define VK_DELETE   0x2e
#define VK_HELP     0x2f

#define VK_NUMPAD0  0x60
#define VK_NUMPAD1  0x61
#define VK_NUMPAD2  0x62
#define VK_NUMPAD3  0x63
#define VK_NUMPAD4  0x64
#define VK_NUMPAD5  0x65
#define VK_NUMPAD6  0x66
#define VK_NUMPAD7  0x67
#define VK_NUMPAD8  0x68
#define VK_NUMPAD9  0x69
#define VK_MULTIPLY 0x6A
#define VK_ADD      0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL  0x6E
#define VK_DIVIDE   0x6F

#define VK_F1       0x70
#define VK_F2       0x71
#define VK_F3       0x72
#define VK_F4       0x73
#define VK_F5       0x74
#define VK_F6       0x75
#define VK_F7       0x76
#define VK_F8       0x77
#define VK_F9       0x78
#define VK_F10      0x79
#define VK_F11      0x7a
#define VK_F12      0x7b
#define VK_F13      0x7c
#define VK_F14      0x7d
#define VK_F15      0x7e
#define VK_F16      0x7f
