/****************************** Module Header ******************************\
* Module Name: messages.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Contains the message indirection table. This is included in both the client
* and server code.
*
* 04-11-91 ScottLu      Created.
\***************************************************************************/

#include "msgdef.h"

#define IMSG_EMPTY      IMSG_DWORD
#define IMSG_RESERVED   IMSG_DWORD

/*
 * Allow posting of LB_DIR and CB_DIR because DlgDirList allows a DDL_POSTMSGS
 * flag that makes the API post the messages.  This should be as long as we
 * don't handle these messages in the kernel.  NT 3.51 allowed posting these.
 */

CONST MSG_TABLE_ENTRY MessageTable[] = {
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NULL                  0x0000
    {IMSG_INLPCREATESTRUCT,  TRUE,  TRUE},        // WM_CREATE                0x0001
    {IMSG_DWORD, FALSE, FALSE},                   // WM_DESTROY               0x0002
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MOVE                  0x0003
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SIZEWAIT              0x0004
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SIZE                  0x0005
    {IMSG_DWORD, FALSE, FALSE},                   // WM_ACTIVATE              0x0006
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SETFOCUS              0x0007
    {IMSG_DWORD, FALSE, FALSE},                   // WM_KILLFOCUS             0x0008
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SETVISIBLE            0x0009
    {IMSG_DWORD, FALSE, FALSE},                   // WM_ENABLE                0x000A
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SETREDRAW             0x000B
    {IMSG_INSTRINGNULL,  TRUE,  TRUE},            // WM_SETTEXT               0x000C
    {IMSG_OUTSTRING,  TRUE,  TRUE},               // WM_GETTEXT               0x000D
    {IMSG_GETDBCSTEXTLENGTHS,  TRUE,  TRUE},      // WM_GETTEXTLENGTH         0x000E
    {IMSG_DWORD, FALSE, FALSE},                   // WM_PAINT                 0x000F

    {IMSG_DWORD, FALSE, FALSE},                   // WM_CLOSE                 0x0010
    {IMSG_DWORD, FALSE, FALSE},                   // WM_QUERYENDSESSION       0x0011
    {IMSG_DWORD, FALSE, FALSE},                   // WM_QUIT                  0x0012
    {IMSG_DWORD, FALSE, FALSE},                   // WM_QUERYOPEN             0x0013
    {IMSG_DWORD, FALSE,  TRUE},                   // WM_ERASEBKGND            0x0014
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SYSCOLORCHANGE        0x0015
    {IMSG_DWORD, FALSE, FALSE},                   // WM_ENDSESSION            0x0016
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SYSTEMERROR           0x0017
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SHOWWINDOW            0x0018
    {IMSG_RESERVED, FALSE, FALSE},                // WM_CTLCOLOR              0x0019
    {IMSG_INSTRINGNULL,  TRUE,  TRUE},            // WM_WININICHANGE          0x001A
    {IMSG_INSTRING,  TRUE,  TRUE},                // WM_DEVMODECHANGE         0x001B
    {IMSG_DWORD, FALSE, FALSE},                   // WM_ACTIVATEAPP           0x001C
    {IMSG_DWORD, FALSE, FALSE},                   // WM_FONTCHANGE            0x001D
    {IMSG_DWORD, FALSE, FALSE},                   // WM_TIMECHANGE            0x001E
    {IMSG_DWORD, FALSE, FALSE},                   // WM_CANCELMODE            0x001F

    {IMSG_DWORD, FALSE, FALSE},                   // WM_SETCURSOR             0x0020
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MOUSEACTIVATE         0x0021
    {IMSG_DWORD, FALSE, FALSE},                   // WM_CHILDACTIVATE         0x0022
    {IMSG_DWORD, FALSE, FALSE},                   // WM_QUEUESYNC             0x0023
    {IMSG_INOUTLPPOINT5, FALSE,  TRUE},           // WM_GETMINMAXINFO         0x0024
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x0025
    {IMSG_DWORD, FALSE, FALSE},                   // WM_PAINTICON             0x0026
    {IMSG_DWORD, FALSE,  TRUE},                   // WM_ICONERASEBKGND        0x0027
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NEXTDLGCTL            0x0028
    {IMSG_DWORD, FALSE, FALSE},                   // WM_ALTTABACTIVE          0x0029
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SPOOLERSTATUS         0x002A
    {IMSG_INLPDRAWITEMSTRUCT, FALSE,  TRUE},      // WM_DRAWITEM              0x002B
    {IMSG_INOUTLPMEASUREITEMSTRUCT, FALSE,  TRUE},// WM_MEASUREITEM           0x002C
    {IMSG_INLPDELETEITEMSTRUCT, FALSE,  TRUE},    // WM_DELETEITEM            0x002D
    {IMSG_DWORD, FALSE, FALSE},                   // WM_VKEYTOITEM            0x002E
    {IMSG_INWPARAMCHAR,  TRUE, FALSE},            // WM_CHARTOITEM            0x002F

    {IMSG_DWORD, FALSE, FALSE},                   // WM_SETFONT               0x0030
    {IMSG_DWORD, FALSE,  TRUE},                   // WM_GETFONT               0x0031
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SETHOTKEY             0x0032
    {IMSG_DWORD, FALSE, FALSE},                   // WM_GETHOTKEY             0x0033
    {IMSG_DWORD, FALSE, FALSE},                   // WM_FILESYSCHANGE         0x0034
    {IMSG_DWORD, FALSE, FALSE},                   // WM_ISACTIVEICON          0x0035
    {IMSG_DWORD, FALSE, FALSE},                   // WM_QUERYPARKICON         0x0036
    {IMSG_DWORD, FALSE, FALSE},                   // WM_QUERYDRAGICON         0x0037
    {IMSG_INLPHLPSTRUCT, FALSE,  TRUE},           // WM_WINHELP               0x0038
    {IMSG_INLPCOMPAREITEMSTRUCT, FALSE,  TRUE},   // WM_COMPAREITEM           0x0039
    {IMSG_DWORD, FALSE, FALSE},                   // WM_FULLSCREEN            0x003A
    {IMSG_DWORD, FALSE, FALSE},                   // WM_CLIENTSHUTDOWN        0x003B
    {IMSG_KERNELONLY, FALSE, TRUE},               // WM_DDEMLEVENT            0x003C
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x003D
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x003E
    {IMSG_DWORD, FALSE, FALSE},                   // MM_CALCSCROLL            0x003F

    {IMSG_RESERVED, FALSE, FALSE},                // WM_TESTING               0x0040
    {IMSG_DWORD, FALSE, FALSE},                   // WM_COMPACTING            0x0041

    {IMSG_RESERVED, FALSE, FALSE},                // WM_OTHERWINDOWCREATED    0x0042
    {IMSG_RESERVED, FALSE, FALSE},                // WM_OTHERWINDOWDESTROYED  0x0043
    {IMSG_RESERVED, FALSE, FALSE},                // WM_COMMNOTIFY            0x0044
    {IMSG_RESERVED, FALSE, FALSE},                // WM_MEDIASTATUSCHANGE     0x0045
    {IMSG_INOUTLPWINDOWPOS, FALSE,  TRUE},        // WM_WINDOWPOSCHANGING     0x0046
    {IMSG_INLPWINDOWPOS, FALSE,  TRUE},           // WM_WINDOWPOSCHANGED      0x0047

    {IMSG_RESERVED, FALSE, FALSE},                // WM_POWER                 0x0048
    {IMSG_COPYGLOBALDATA,  TRUE,  TRUE},          // WM_COPYGLOBALDATA        0x0049
    {IMSG_COPYDATA, FALSE,  TRUE},                // WM_COPYDATA              0x004A
    {IMSG_RESERVED, FALSE, FALSE},                // WM_CANCELJOURNAL         0x004B
    {IMSG_LOGONNOTIFY, FALSE, FALSE},             // WM_LOGONNOTIFY           0x004C
    {IMSG_DWORD, FALSE, FALSE},                   // WM_KEYF1                 0x004D
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NOTIFY                0x004E
    {IMSG_RESERVED, FALSE, FALSE},                // WM_ACCESS_WINDOW         0x004f

    {IMSG_DWORD, FALSE, FALSE},                   // WM_INPUTLANGCHANGEREQUEST 0x0050
    {IMSG_DWORD, FALSE, FALSE},                   // WM_INPUTLANGCHANGE       0x0051
    {IMSG_EMPTY, FALSE, FALSE},                   // WM_TCARD                 0x0052
    {IMSG_INLPHELPINFOSTRUCT, FALSE,  TRUE},      // WM_HELP                  0x0053 WINHELP4
    {IMSG_EMPTY, FALSE, FALSE},                   // WM_USERCHANGED           0x0054
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NOTIFYFORMAT          0x0055
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0059-0x005F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0060-0x0067
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0068-0x006F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_KERNELONLY, FALSE, TRUE},               // WM_FINALDESTROY          0x0070
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_DWORD, FALSE, FALSE},                   // WM_TASKACTIVATED         0x0072
    {IMSG_DWORD, FALSE, FALSE},                   // WM_TASKDEACTIVATED       0x0073
    {IMSG_DWORD, FALSE, FALSE},                   // WM_TASKCREATED           0x0074
    {IMSG_DWORD, FALSE, FALSE},                   // WM_TASKDESTROYED         0x0075
    {IMSG_DWORD, FALSE, FALSE},                   // WM_TASKUICHANGED         0x0076
    {IMSG_DWORD, FALSE, FALSE},                   // WM_TASKVISIBLE           0x0077
    {IMSG_DWORD, FALSE, FALSE},                   // WM_TASKNOTVISIBLE        0x0078
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SETCURSORINFO         0x0079
    {IMSG_EMPTY, FALSE, FALSE},                   //                          0x007A
    {IMSG_DWORD, FALSE, FALSE},                   // WM_CONTEXTMENU           0x007B
    {IMSG_INOUTSTYLECHANGE, FALSE,  TRUE},        // WM_STYLECHANGING         0x007C
    {IMSG_INOUTSTYLECHANGE, FALSE,  TRUE},        // WM_STYLECHANGED          0x007D
    {IMSG_EMPTY, FALSE, FALSE},                   //                          0x007E
    {IMSG_DWORD, FALSE, FALSE},                   // WM_GETICON               0x007f

    {IMSG_DWORD, FALSE, FALSE},                   // WM_SETICON               0x0080
    {IMSG_INLPCREATESTRUCT,  TRUE,  TRUE},        // WM_NCCREATE              0x0081
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCDESTROY             0x0082
    {IMSG_INOUTNCCALCSIZE, FALSE,  TRUE},         // WM_NCCALCSIZE            0x0083

    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCHITTEST             0x0084
    {IMSG_DWORD, FALSE,  TRUE},                   // WM_NCPAINT               0x0085
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCACTIVATE            0x0086
    {IMSG_DWORDOPTINLPMSG, FALSE,  TRUE},         // WM_GETDLGCODE            0x0087

    {IMSG_DWORD, FALSE, FALSE},                   // WM_SYNCPAINT             0x0088
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SYNCTASK              0x0089

    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_INOUTLPRECT, FALSE,  TRUE},             // WM_KLUDGEMINRECT         0x008B
    {IMSG_INLPKDRAWSWITCHWND, FALSE, TRUE},       // WM_LPKDRAWSWITCHWND      0x008C
    {IMSG_EMPTY, FALSE, FALSE},                   // 0x008D-0x008F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0090-0x0097
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0098-0x009F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCMOUSEMOVE           0x00A0
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCLBUTTONDOWN         0x00A1
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCLBUTTONUP           0x00A2
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCLBUTTONDBLCLK       0x00A3
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCRBUTTONDOWN         0x00A4
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCRBUTTONUP           0x00A5
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCRBUTTONDBLCLK       0x00A6
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCMBUTTONDOWN         0x00A7
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCMBUTTONUP           0x00A8
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCMBUTTONDBLCLK       0x00A9

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x00AA-0x00AF
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMGETSEL, FALSE,  TRUE},                // EM_GETSEL                0x00B0
    {IMSG_EMSETSEL, FALSE, FALSE},                // EM_SETSEL                0x00B1
    {IMSG_OUTLPRECT, FALSE,  TRUE},               // EM_GETRECT               0x00B2
    {IMSG_INOUTLPRECT, FALSE,  TRUE},             // EM_SETRECT               0x00B3
    {IMSG_INOUTLPRECT, FALSE,  TRUE},             // EM_SETRECTNP             0x00B4
    {IMSG_DWORD, FALSE, FALSE},                   // EM_SCROLL                0x00B5
    {IMSG_DWORD, FALSE, FALSE},                   // EM_LINESCROLL            0x00B6
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x00B7
    {IMSG_DWORD, FALSE, FALSE},                   // EM_GETMODIFY             0x00B8
    {IMSG_DWORD, FALSE, FALSE},                   // EM_SETMODIFY             0x00B9
    {IMSG_DWORD, FALSE, FALSE},                   // EM_GETLINECOUNT          0x00BA
    {IMSG_DWORD, FALSE, FALSE},                   // EM_LINEINDEX             0x00BB
    {IMSG_DWORD, FALSE, FALSE},                   // EM_SETHANDLE             0x00BC
    {IMSG_DWORD, FALSE, FALSE},                   // EM_GETHANDLE             0x00BD
    {IMSG_DWORD, FALSE, FALSE},                   // EM_GETTHUMB              0x00BE
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x00BF

    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x00C0
    {IMSG_DWORD, FALSE, FALSE},                   // EM_LINELENGTH            0x00C1
    {IMSG_INSTRINGNULL,  TRUE,  TRUE},            // EM_REPLACESEL            0x00C2
    {IMSG_DWORD, FALSE, FALSE},                   // EM_SETFONT               0x00C3
    {IMSG_INCNTOUTSTRING,  TRUE,  TRUE},          // EM_GETLINE               0x00C4
    {IMSG_DWORD, FALSE, FALSE},                   // EM_LIMITTEXT             0x00C5
    {IMSG_DWORD, FALSE, FALSE},                   // EM_CANUNDO               0x00C6
    {IMSG_DWORD, FALSE, FALSE},                   // EM_UNDO                  0x00C7
    {IMSG_DWORD, FALSE, FALSE},                   // EM_FMTLINES              0x00C8
    {IMSG_DWORD, FALSE, FALSE},                   // EM_LINEFROMCHAR          0x00C9
    {IMSG_DWORD, FALSE, FALSE},                   // EM_SETWORDBREAK          0x00CA
    {IMSG_POPTINLPUINT, FALSE,  TRUE},            // EM_SETTABSTOPS           0x00CB
    {IMSG_INWPARAMDBCSCHAR,  TRUE, FALSE},        // EM_SETPASSWORDCHAR       0x00CC
    {IMSG_DWORD, FALSE, FALSE},                   // EM_EMPTYUNDOBUFFER       0x00CD
    {IMSG_DWORD, FALSE, FALSE},                   // EM_GETFIRSTVISIBLELINE   0x00CE
    {IMSG_DWORD, FALSE, FALSE},                   // EM_SETREADONLY           0x00CF

    {IMSG_DWORD, FALSE, FALSE},                   // EM_SETWORDBREAKPROC      0x00D0
    {IMSG_DWORD, FALSE, FALSE},                   // EM_GETWORDBREAKPROC      0x00D1
    {IMSG_DWORD, FALSE, FALSE},                   // EM_GETPASSWORDCHAR       0x00D2
    {IMSG_DWORD, FALSE, FALSE},                   // EM_SETMARGINS            0x00D3
    {IMSG_DWORD, FALSE, FALSE},                   // EM_GETMARGINS            0x00D4
    {IMSG_DWORD, FALSE, FALSE},                   // EM_GETLIMITTEXT          0x00D5
    {IMSG_DWORD, FALSE, FALSE},                   // EM_POSFROMCHAR           0x00D6
    {IMSG_DWORD, FALSE, FALSE},                   // EM_CHARFROMPOS           0x00D7
    {IMSG_DWORD, FALSE, FALSE},                   // EM_SETIMESTATUS          0x00D8

    {IMSG_DWORD, FALSE, FALSE},                   // EM_GETIMESTATUS          0x00D9
    {IMSG_RESERVED, FALSE, FALSE},                // EM_MSGMAX                0x00DA
    {IMSG_EMPTY, FALSE, FALSE},                   // 0x00DB-0x00DF
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_DWORD, FALSE, FALSE},                   // SBM_SETPOS               0x00E0
    {IMSG_DWORD, FALSE, FALSE},                   // SBM_GETPOS               0x00E1
    {IMSG_DWORD, FALSE, FALSE},                   // SBM_SETRANGE             0x00E2
    {IMSG_OPTOUTLPDWORDOPTOUTLPDWORD, FALSE,  TRUE}, // SBM_GETRANGE          0x00E3
    {IMSG_DWORD, FALSE, FALSE},                   // SBM_ENABLE_ARROWS        0x00E4
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_DWORD, FALSE, FALSE},                   // SBM_SETRANGEREDRAW       0x00E6
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_INOUTLPSCROLLINFO, FALSE,  TRUE},       // SBM_SETSCROLLINFO        0x00E9
    {IMSG_INOUTLPSCROLLINFO, FALSE,  TRUE},       // SBM_GETSCROLLINFO        0x00EA
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_DWORD, FALSE, FALSE},                   // BM_GETCHECK              0x00F0
    {IMSG_DWORD, FALSE, FALSE},                   // BM_SETCHECK              0x00F1
    {IMSG_DWORD, FALSE, FALSE},                   // BM_GETSTATE              0x00F2
    {IMSG_DWORD, FALSE, FALSE},                   // BM_SETSTATE              0x00F3
    {IMSG_DWORD, FALSE, FALSE},                   // BM_SETSTYLE              0x00F4
    {IMSG_DWORD, FALSE, FALSE},                   // BM_CLICK                 0x00F5
    {IMSG_DWORD, FALSE, FALSE},                   // BM_GETIMAGE              0x00F6
    {IMSG_DWORD, FALSE, FALSE},                   // BM_SETIMAGE              0x00F7

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x00F8-0x00FF
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_DWORD, FALSE, FALSE},                   // WM_KEYDOWN               0x0100
    {IMSG_DWORD, FALSE, FALSE},                   // WM_KEYUP                 0x0101
    {IMSG_INWPARAMDBCSCHAR,  TRUE, FALSE},        // WM_CHAR                  0x0102
    {IMSG_INWPARAMCHAR,  TRUE, FALSE},            // WM_DEADCHAR              0x0103
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SYSKEYDOWN            0x0104
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SYSKEYUP              0x0105
    {IMSG_INWPARAMCHAR,  TRUE, FALSE},            // WM_SYSCHAR               0x0106
    {IMSG_INWPARAMCHAR,  TRUE, FALSE},            // WM_SYSDEADCHAR           0x0107
    {IMSG_DWORD, FALSE,  TRUE},                   // WM_YOMICHAR              0x0108
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x0109
    {IMSG_RESERVED, FALSE,  TRUE},                // WM_CONVERTREQUEST        0x010A
    {IMSG_RESERVED, FALSE, FALSE},                // WM_CONVERTRESULT         0x010B
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x010C
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x010D
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x010E
    {IMSG_INWPARAMCHAR,  TRUE, FALSE},            // WM_IME_COMPOSITION       0x010F

    {IMSG_DWORD, FALSE, FALSE},                   // WM_INITDIALOG            0x0110
    {IMSG_DWORD, FALSE, FALSE},                   // WM_COMMAND               0x0111
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SYSCOMMAND            0x0112
    {IMSG_DWORD, FALSE, FALSE},                   // WM_TIMER                 0x0113
    {IMSG_DWORD, FALSE, FALSE},                   // WM_HSCROLL               0x0114
    {IMSG_DWORD, FALSE, FALSE},                   // WM_VSCROLL               0x0115
    {IMSG_DWORD, FALSE, FALSE},                   // WM_INITMENU              0x0116
    {IMSG_DWORD, FALSE, FALSE},                   // WM_INITMENUPOPUP         0x0117
    {IMSG_DWORD, FALSE, FALSE},                   // WM_SYSTIMER              0x0118
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x0119
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x011A
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x011B
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x011C
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x011D
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x011E
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MENUSELECT            0x011F

    {IMSG_INWPARAMCHAR,  TRUE, FALSE},            // WM_MENUCHAR              0x0120
    {IMSG_DWORD, FALSE, FALSE},                   // WM_ENTERIDLE             0x0121
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MENURBUTTONUP         0x0122
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MENUDRAG              0x0123
    {IMSG_INOUTMENUGETOBJECT, TRUE, TRUE},        // WM_MENUGETOBJECT         0x0124
    {IMSG_DWORD, FALSE, FALSE},                   // WM_UNINITMENUPOPUP       0x0125
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MENUCOMMAND           0x0126
    {IMSG_DWORD, FALSE, FALSE},                   // WM_CHANGEUISTATE         0x0127
    {IMSG_DWORD, FALSE, FALSE},                   // WM_UPDATEUISTATE         0x0128
    {IMSG_DWORD, FALSE, FALSE},                   // WM_QUERYUISTATE          0x0129

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x012A-0x012F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x0130
    {IMSG_DWORD, FALSE, FALSE},                   // WM_LBTRACKPOINT          0x0131
    {IMSG_DWORD, FALSE,  TRUE},                   // WM_CTLCOLORMSGBOX        0x0132
    {IMSG_DWORD, FALSE,  TRUE},                   // WM_CTLCOLOREDIT          0x0133
    {IMSG_DWORD, FALSE,  TRUE},                   // WM_CTLCOLORLISTBOX       0x0134
    {IMSG_DWORD, FALSE,  TRUE},                   // WM_CTLCOLORBTN           0x0135
    {IMSG_DWORD, FALSE,  TRUE},                   // WM_CTLCOLORDLG           0x0136
    {IMSG_DWORD, FALSE,  TRUE},                   // WM_CTLCOLORSCROLLBAR     0x0137
    {IMSG_DWORD, FALSE,  TRUE},                   // WM_CTLCOLORSTATIC        0x0138
    {IMSG_EMPTY, FALSE, FALSE},                   //                          0x0139

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x013A-0x013F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_CBGETEDITSEL, FALSE,  TRUE},            // CB_GETEDITSEL            0x0140
    {IMSG_DWORD, FALSE, FALSE},                   // CB_LIMITTEXT             0x0141
    {IMSG_DWORD, FALSE, FALSE},                   // CB_SETEDITSEL            0x0142
    {IMSG_INCBOXSTRING,  TRUE,  TRUE},            // CB_ADDSTRING             0x0143
    {IMSG_DWORD, FALSE, FALSE},                   // CB_DELETESTRING          0x0144
    {IMSG_INSTRING,  TRUE,  FALSE},               // CB_DIR                   0x0145
    {IMSG_DWORD, FALSE, FALSE},                   // CB_GETCOUNT              0x0146
    {IMSG_DWORD, FALSE, FALSE},                   // CB_GETCURSEL             0x0147
    {IMSG_OUTCBOXSTRING,  TRUE,  TRUE},           // CB_GETLBTEXT             0x0148
    {IMSG_GETDBCSTEXTLENGTHS,  TRUE,  TRUE},      // CB_GETLBTEXTLEN          0x0149
    {IMSG_INCBOXSTRING,  TRUE,  TRUE},            // CB_INSERTSTRING          0x014A
    {IMSG_DWORD, FALSE, FALSE},                   // CB_RESETCONTENT          0x014B
    {IMSG_INCBOXSTRING,  TRUE,  TRUE},            // CB_FINDSTRING            0x014C
    {IMSG_INCBOXSTRING,  TRUE,  TRUE},            // CB_SELECTSTRING          0x014D
    {IMSG_DWORD, FALSE, FALSE},                   // CB_SETCURSEL             0x014E
    {IMSG_DWORD, FALSE, FALSE},                   // CB_SHOWDROPDOWN          0x014F

    {IMSG_DWORD, FALSE, FALSE},                   // CB_GETITEMDATA           0x0150
    {IMSG_DWORD, FALSE, FALSE},                   // CB_SETITEMDATA           0x0151
    {IMSG_OUTLPRECT, FALSE,  TRUE},               // CB_GETDROPPEDCONTROLRECT 0x0152
    {IMSG_DWORD, FALSE, FALSE},                   // CB_SETITEMHEIGHT         0x0153
    {IMSG_DWORD, FALSE, FALSE},                   // CB_GETITEMHEIGHT         0x0154
    {IMSG_DWORD, FALSE, FALSE},                   // CB_SETEXTENDEDUI         0x0155
    {IMSG_DWORD, FALSE, FALSE},                   // CB_GETEXTENDEDUI         0x0156
    {IMSG_DWORD, FALSE, FALSE},                   // CB_GETDROPPEDSTATE       0x0157
    {IMSG_INCBOXSTRING,  TRUE,  TRUE},            // CB_FINDSTRINGEXACT       0x0158
    {IMSG_DWORD, FALSE, FALSE},                   // CB_SETLOCALE             0x0159
    {IMSG_DWORD, FALSE, FALSE},                   // CB_GETLOCALE             0x015A
    {IMSG_DWORD, FALSE, FALSE},                   // CB_GETTOPINDEX           0x015b

    {IMSG_DWORD, FALSE, FALSE},                   // CB_SETTOPINDEX           0x015c
    {IMSG_DWORD, FALSE, FALSE},                   // CB_GETHORIZONTALEXTENT   0x015d
    {IMSG_DWORD, FALSE, FALSE},                   // CB_SETHORIZONTALEXTENT   0x015e
    {IMSG_DWORD, FALSE, FALSE},                   // CB_GETDROPPEDWIDTH       0x015F

    {IMSG_DWORD, FALSE, FALSE},                   // CB_SETDROPPEDWIDTH       0x0160
    {IMSG_DWORD, FALSE, FALSE},                   // CB_INITSTORAGE           0x0161
    {IMSG_RESERVED, FALSE, FALSE},                // CB_MSGMAX                0x0162
    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0163-0x0167
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0168-0x016F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_DWORD, FALSE, FALSE},                   // STM_SETICON              0x0170
    {IMSG_DWORD, FALSE, FALSE},                   // STM_GETICON              0x0171
    {IMSG_DWORD, FALSE, FALSE},                   // STM_SETIMAGE             0x0172
    {IMSG_DWORD, FALSE, FALSE},                   // STM_GETIMAGE             0x0173
    {IMSG_DWORD, FALSE, FALSE},                   // STM_MSGMAX               0x0174
    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0175-0x0177
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0178-0x017F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_INLBOXSTRING,  TRUE,  TRUE},            // LB_ADDSTRING             0x0180
    {IMSG_INLBOXSTRING,  TRUE,  TRUE},            // LB_INSERTSTRING          0x0181
    {IMSG_DWORD, FALSE, FALSE},                   // LB_DELETESTRING          0x0182
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x0183
    {IMSG_DWORD, FALSE, FALSE},                   // LB_RESETCONTENT          0x0184
    {IMSG_DWORD, FALSE, FALSE},                   // LB_SETSEL                0x0185
    {IMSG_DWORD, FALSE, FALSE},                   // LB_SETCURSEL             0x0186
    {IMSG_DWORD, FALSE, FALSE},                   // LB_GETSEL                0x0187
    {IMSG_DWORD, FALSE, FALSE},                   // LB_GETCURSEL             0x0188
    {IMSG_OUTLBOXSTRING,  TRUE,  TRUE},           // LB_GETTEXT               0x0189
    {IMSG_GETDBCSTEXTLENGTHS,  TRUE,  TRUE},      // LB_GETTEXTLEN            0x018A
    {IMSG_DWORD, FALSE, FALSE},                   // LB_GETCOUNT              0x018B
    {IMSG_INLBOXSTRING,  TRUE,  TRUE},            // LB_SELECTSTRING          0x018C
    {IMSG_INSTRING,  TRUE,  FALSE},               // LB_DIR                   0x018D
    {IMSG_DWORD, FALSE, FALSE},                   // LB_GETTOPINDEX           0x018E
    {IMSG_INLBOXSTRING,  TRUE,  TRUE},            // LB_FINDSTRING            0x018F

    {IMSG_DWORD, FALSE, FALSE},                   // LB_GETSELCOUNT           0x0190
    {IMSG_POUTLPINT, FALSE,  TRUE},               // LB_GETSELITEMS           0x0191
    {IMSG_POPTINLPUINT, FALSE,  TRUE},            // LB_SETTABSTOPS           0x0192
    {IMSG_DWORD, FALSE, FALSE},                   // LB_GETHORIZONTALEXTENT   0x0193
    {IMSG_DWORD, FALSE, FALSE},                   // LB_SETHORIZONTALEXTENT   0x0194
    {IMSG_DWORD, FALSE, FALSE},                   // LB_SETCOLUMNWIDTH        0x0195
    {IMSG_INSTRING,  TRUE,  TRUE},                // LB_ADDFILE               0x0196
    {IMSG_DWORD, FALSE, FALSE},                   // LB_SETTOPINDEX           0x0197
    {IMSG_INOUTLPRECT, FALSE,  TRUE},             // LB_GETITEMRECT           0x0198
    {IMSG_DWORD, FALSE, FALSE},                   // LB_GETITEMDATA           0x0199
    {IMSG_DWORD, FALSE, FALSE},                   // LB_SETITEMDATA           0x019A
    {IMSG_DWORD, FALSE, FALSE},                   // LB_SELITEMRANGE          0x019B
    {IMSG_DWORD, FALSE, FALSE},                   // LB_SETANCHORINDEX        0x019C
    {IMSG_DWORD, FALSE, FALSE},                   // LB_GETANCHORINDEX        0x019D
    {IMSG_DWORD, FALSE, FALSE},                   // LB_SETCARETINDEX         0x019E
    {IMSG_DWORD, FALSE, FALSE},                   // LB_GETCARETINDEX         0x019F

    {IMSG_DWORD, FALSE, FALSE},                   // LB_SETITEMHEIGHT         0x01A0
    {IMSG_DWORD, FALSE, FALSE},                   // LB_GETITEMHEIGHT         0x01A1
    {IMSG_INLBOXSTRING,  TRUE,  TRUE},            // LB_FINDSTRINGEXACT       0x01A2
    {IMSG_DWORD, FALSE, FALSE},                   // LBCB_CARETON             0x01A3
    {IMSG_DWORD, FALSE, FALSE},                   // LBCB_CARETOFF            0x01A4
    {IMSG_DWORD, FALSE, FALSE},                   // LB_SETLOCALE             0x01A5
    {IMSG_DWORD, FALSE, FALSE},                   // LB_GETLOCALE             0x01A6
    {IMSG_DWORD, FALSE, FALSE},                   // LB_SETCOUNT              0x01A7

    {IMSG_DWORD, FALSE, FALSE},                   // LB_INITSTORAGE           0x01A8

    {IMSG_DWORD, FALSE, FALSE},                   // LB_ITEMFROMPOINT         0x01A9
    {IMSG_INLBOXSTRING,  TRUE,  TRUE},            // LB_INSERTSTRINGUPPER     0x01AA
    {IMSG_INLBOXSTRING,  TRUE,  TRUE},            // LB_INSERTSTRINGLOWER     0x01AB
    {IMSG_INLBOXSTRING,  TRUE,  TRUE},            // LB_ADDSTRINGUPPER        0x01AC
    {IMSG_INLBOXSTRING,  TRUE,  TRUE},            // LB_ADDSTRINGLOWER        0x01AD
    {IMSG_DWORD, FALSE, FALSE},                   // LBCB_STARTTRACK          0x01AE
    {IMSG_DWORD, FALSE, FALSE},                   // LBCB_ENDTRACK            0x01AF

    {IMSG_RESERVED, FALSE, FALSE},                // LB_MSGMAX                0x01B0
    {IMSG_EMPTY, FALSE, FALSE},                   // 0x01B1-0x01B7
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x01B8-0x01BF
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x01C0-0x01C7
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x01C8-0x01CF
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x01D0-0x01D7
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x01D8-0x01DF
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_DWORD, FALSE, FALSE},                   // MN_SETHMENU              0x01E0
    {IMSG_DWORD, FALSE, FALSE},                   // MN_GETHMENU              0x01E1
    {IMSG_DWORD, FALSE, FALSE},                   // MN_SIZEWINDOW            0x01E2
    {IMSG_DWORD, FALSE, FALSE},                   // MN_OPENHIERARCHY         0x01E3
    {IMSG_DWORD, FALSE, FALSE},                   // MN_CLOSEHIERARCHY        0x01E4
    {IMSG_DWORD, FALSE, FALSE},                   // MN_SELECTITEM            0x01E5
    {IMSG_DWORD, FALSE, FALSE},                   // MN_CANCELMENUS           0x01E6
    {IMSG_DWORD, FALSE, FALSE},                   // MN_SELECTFIRSTVALIDITEM  0x01E7

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x1E8 - 0x1E9
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},                   // MN_GETPPOPUPMENU(obsolete) 0x01EA
    {IMSG_OUTDWORDINDWORD, FALSE,  TRUE},         // MN_FINDMENUWINDOWFROMPOINT 0x01EB
    {IMSG_DWORD, FALSE, FALSE},                   // MN_SHOWPOPUPWINDOW         0x01EC
    {IMSG_DWORD, FALSE, FALSE},                   // MN_BUTTONDOWN              0x01ED
    {IMSG_DWORD, FALSE, FALSE},                   // MN_MOUSEMOVE               0x01EE
    {IMSG_DWORD, FALSE, FALSE},                   // MN_BUTTONUP                0x01EF
    {IMSG_DWORD, FALSE, FALSE},                   // MN_SETTIMERTOOPENHIERARCHY 0x01F0

    {IMSG_DWORD, FALSE, FALSE},                   // MN_DBLCLK                  0x01F1
    {IMSG_DWORD, FALSE, FALSE},                   // MN_ENDMENU                 0x01F2
    {IMSG_DWORD, FALSE, FALSE},                   // MN_DODRAGDROP              0x01F3
    {IMSG_DWORD, FALSE, FALSE},                   // MN_ENDMENU                 0x01F4

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x01F5-0x01F7
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x01F8-0x01FF
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_DWORD, FALSE, FALSE},                   // WM_MOUSEMOVE             0x0200
    {IMSG_DWORD, FALSE, FALSE},                   // WM_LBUTTONDOWN           0x0201
    {IMSG_DWORD, FALSE, FALSE},                   // WM_LBUTTONUP             0x0202
    {IMSG_DWORD, FALSE, FALSE},                   // WM_LBUTTONDBLCLK         0x0203
    {IMSG_DWORD, FALSE, FALSE},                   // WM_RBUTTONDOWN           0x0204
    {IMSG_DWORD, FALSE, FALSE},                   // WM_RBUTTONUP             0x0205
    {IMSG_DWORD, FALSE, FALSE},                   // WM_RBUTTONDBLCLK         0x0206
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MBUTTONDOWN           0x0207
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MBUTTONUP             0x0208
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MBUTTONDBLCLK         0x0209
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MOUSEWHEEL            0x020A
    {IMSG_DWORD, FALSE, FALSE},                   // WM_XBUTTONDOWN           0x020B
    {IMSG_DWORD, FALSE, FALSE},                   // WM_XBUTTONUP             0x020C
    {IMSG_DWORD, FALSE, FALSE},                   // WM_XBUTTONDBLCLK         0x020D
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x020E
    {IMSG_EMPTY, FALSE, FALSE},                   // empty                    0x020F

    {IMSG_DWORD, FALSE,  TRUE},                   // WM_PARENTNOTIFY          0x0210
    {IMSG_DWORD, FALSE, FALSE},                   // WM_ENTERMENULOOP         0x0211
    {IMSG_DWORD, FALSE, FALSE},                   // WM_EXITMENULOOP          0x0212
    {IMSG_INOUTNEXTMENU, FALSE,  TRUE},           // WM_NEXTMENU              0x0213

    {IMSG_INOUTLPRECT, FALSE,  TRUE},             // WM_SIZING                0x0214
    {IMSG_DWORD, FALSE, FALSE},                   // WM_CAPTURECHANGED        0x0215
    {IMSG_INOUTLPRECT, FALSE,  TRUE},             // WM_MOVING                0x0216
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_POWERBROADCAST, FALSE, FALSE},          // WM_POWERBROADCAST        0x0218
    {IMSG_INDEVICECHANGE, FALSE, FALSE},          // WM_DEVICECHANGE          0x0219
    {IMSG_EMPTY, FALSE, FALSE},                   // 0x021A-0x021F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_INLPMDICREATESTRUCT,  TRUE,  TRUE},     // WM_MDICREATE             0x0220
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MDIDESTROY            0x0221
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MDIACTIVATE           0x0222
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MDIRESTORE            0x0223
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MDINEXT               0x0224
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MDIMAXIMIZE           0x0225
    {IMSG_RESERVED, FALSE, FALSE},                // WM_MDITILE               0x0226
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MDICASCADE            0x0227
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MDIICONARRANGE        0x0228
    {IMSG_OPTOUTLPDWORDOPTOUTLPDWORD, FALSE,  TRUE}, // WM_MDIGETACTIVE       0x0229
    {IMSG_INOUTDRAG, FALSE,  TRUE},               // WM_DROPOBJECT            0x022A
    {IMSG_INOUTDRAG, FALSE,  TRUE},               // WM_QUERYDROPOBJECT       0x022B
    {IMSG_DWORD, FALSE, FALSE},                   // WM_BEGINDRAG             0x022C
    {IMSG_INOUTDRAG, FALSE,  TRUE},               // WM_DRAGLOOP              0x022D
    {IMSG_INOUTDRAG, FALSE,  TRUE},               // WM_DRAGSELECT            0x022E
    {IMSG_INOUTDRAG, FALSE,  TRUE},               // WM_DRAGMOVE              0x022F

    {IMSG_DWORD, FALSE, FALSE},                   // WM_MDISETMENU            0x0230
    {IMSG_DWORD, FALSE, FALSE},                   // WM_ENTERSIZEMOVE         0x0231
    {IMSG_DWORD, FALSE, FALSE},                   // WM_EXITSIZEMOVE          0x0232

    {IMSG_EMPTY, FALSE, FALSE},                   // WM_DROPFILES             0x0233
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MDIREFRESHMENU        0x0234
    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0235-0x0237
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0238-0x023F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0240-0x0247
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0248-0x024F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0250-0x0257
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0258-0x025F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0260-0x0267
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0268-0x026F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0270-0x0277
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0278-0x027F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_DWORD, FALSE, FALSE},                   // WM_IME_REPORT            0x0280
    {IMSG_DWORD, FALSE,  TRUE},                   // WM_IME_SETCONTEXT        0x0281
    {IMSG_DWORD, FALSE, FALSE},                   // WM_IME_NOTIFY            0x0282
    {IMSG_IMECONTROL,  TRUE,  TRUE},              // WM_IME_CONTROL           0x0283
    {IMSG_DWORD, FALSE, FALSE},                   // WM_IME_COMPOSITIONFULL   0x0284
    {IMSG_DWORD, FALSE, FALSE},                   // WM_IME_SELECT            0x0285
    {IMSG_INWPARAMCHAR,  TRUE, FALSE},            // WM_IME_CHAR              0x0286
    {IMSG_RESERVED, FALSE, FALSE},

    {IMSG_RESERVED, FALSE, FALSE},                // 0x0288
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},

    {IMSG_RESERVED, FALSE, FALSE},                // 0x0290
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},

    {IMSG_RESERVED, FALSE, FALSE},                // 0x0298
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},                // WM_KANJILAST             0x029F

    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCMOUSEHOVER          0x02Ao
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MOUSEHOVER            0x02A1
    {IMSG_DWORD, FALSE, FALSE},                   // WM_NCMOUSELEAVE          0x02A2
    {IMSG_DWORD, FALSE, FALSE},                   // WM_MOUSELEAVE            0x02A3

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x02A4-0x02A7
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x02A8-0x02AF
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x02B0-0x02B7
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x02B8-0x02BF
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x02C0-0x02C7
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x02C8-0x02CF
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x02D0-0x02D7
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x02D8-0x02DF
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x02E0-0x02E7
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x02E8-0x02EF
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x02F0-0x02F7
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x02F8-0x02FF
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_DWORD, FALSE, FALSE},                   // WM_CUT                   0x0300
    {IMSG_DWORD, FALSE, FALSE},                   // WM_COPY                  0x0301
    {IMSG_DWORD, FALSE, FALSE},                   // WM_PASTE                 0x0302
    {IMSG_DWORD, FALSE, FALSE},                   // WM_CLEAR                 0x0303
    {IMSG_DWORD, FALSE, FALSE},                   // WM_UNDO                  0x0304
    {IMSG_DWORD, FALSE, FALSE},                   // WM_RENDERFORMAT          0x0305
    {IMSG_INDESTROYCLIPBRD,  TRUE, FALSE},        // WM_RENDERALLFORMATS      0x0306
    {IMSG_INDESTROYCLIPBRD,  TRUE, FALSE},        // WM_DESTROYCLIPBOARD      0x0307
    {IMSG_DWORD, FALSE, FALSE},                   // WM_DRAWCLIPBOARD         0x0308
    {IMSG_INPAINTCLIPBRD,  TRUE,  TRUE},          // WM_PAINTCLIPBOARD        0x0309
    {IMSG_DWORD, FALSE, FALSE},                   // WM_VSCROLLCLIPBOARD      0x030A
    {IMSG_INSIZECLIPBRD,  TRUE,  TRUE},           // WM_SIZECLIPBOARD         0x030B
    {IMSG_INCNTOUTSTRINGNULL,  TRUE,  TRUE},      // WM_ASKCBFORMATNAME       0x030C
    {IMSG_DWORD, FALSE, FALSE},                   // WM_CHANGECBCHAIN         0x030D
    {IMSG_DWORD, FALSE, FALSE},                   // WM_HSCROLLCLIPBOARD      0x030E
    {IMSG_DWORD, FALSE, FALSE},                   // WM_QUERYNEWPALETTE       0x030F

    {IMSG_DWORD, FALSE, FALSE},                   // WM_PALETTEISCHANGING     0x0310
    {IMSG_DWORD, FALSE, FALSE},                   // WM_PALETTECHANGED        0x0311
    {IMSG_DWORD, FALSE, FALSE},                   // WM_HOTKEY                0x0312

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0313-0x0316
    {IMSG_KERNELONLY, FALSE,  TRUE},              // WM_HOOKMSG               0x0314
    {IMSG_EMPTY, FALSE, FALSE},                   // WM_EXITPROCESS           0x0315
    {IMSG_EMPTY, FALSE, FALSE},                   // WM_WAKETHREAD            0x0316
    {IMSG_DWORD, FALSE, FALSE},                   // WM_PRINT                 0x0317

    {IMSG_DWORD, FALSE, FALSE},                   // WM_PRINTCLIENT           0x0318
    {IMSG_DWORD, FALSE, FALSE},                   // WM_APPCOMMAND            0x0319
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0320-0x0327
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0328-0x032F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0330-0x0337
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0338-0x033F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0340-0x0347
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0348-0x034F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0350-0x0357
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // reserved pen windows      0x0358-0x035F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0360-0x0367
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0368-0x036F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0370-0x0377
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0378-0x037F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0380-0x0387
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0388-0x038F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0390-0x0397
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x0398-0x039F
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_RESERVED, FALSE, FALSE},                // WM_MM_RESERVED_FIRST      0x03A0
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},

    {IMSG_RESERVED, FALSE, FALSE},                // 0x03A8
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},

    {IMSG_RESERVED, FALSE, FALSE},                // 0x03B0
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},

    {IMSG_RESERVED, FALSE, FALSE},                // 0x03B7
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},

    {IMSG_RESERVED, FALSE, FALSE},                // 0x03C0
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},

    {IMSG_RESERVED, FALSE, FALSE},                // 0x03C7
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},

    {IMSG_RESERVED, FALSE, FALSE},                // 0x03D0
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},

    {IMSG_RESERVED, FALSE, FALSE},                // 0x03D7
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},                // WM_MM_RESERVED_LAST      0x03DF

    {IMSG_DDEINIT,  TRUE, FALSE},                 // WM_DDE_INITIATE          0x03E0
    {IMSG_DWORD,  TRUE, FALSE},                   // WM_DDE_TERMINATE         0x03E1
    {IMSG_SENTDDEMSG,  TRUE, FALSE},              // WM_DDE_ADVISE            0x03E2
    {IMSG_SENTDDEMSG,  TRUE, FALSE},              // WM_DDE_UNADVISE          0x03E3
    {IMSG_DWORD,  TRUE, FALSE},                   // WM_DDE_ACK               0x03E4
    {IMSG_SENTDDEMSG,  TRUE, FALSE},              // WM_DDE_DATA              0x03E5
    {IMSG_SENTDDEMSG,  TRUE, FALSE},              // WM_DDE_REQUEST           0x03E6
    {IMSG_SENTDDEMSG,  TRUE, FALSE},              // WM_DDE_POKE              0x03E7
    {IMSG_SENTDDEMSG,  TRUE, FALSE},              // WM_DDE_EXECUTE           0x03E8

    {IMSG_EMPTY, FALSE, FALSE},                   // 0x03E9-0x03EF
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},
    {IMSG_EMPTY, FALSE, FALSE},

    {IMSG_RESERVED, FALSE, FALSE},                // WM_CBT_RESERVED_FIRST     0x03F0
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},

    {IMSG_RESERVED, FALSE, FALSE},                // 0x03F8
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},
    {IMSG_RESERVED, FALSE, FALSE},                // WM_CBT_RESERVED_LAST      0x03FF
};

