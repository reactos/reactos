/* ComboBox Control Test for ReactOS.

* This is a test program. Not made to be fast, small
* easy to mantain, or portable.

* I'm not erasing text because I don't want to use other functions from the API
* or make this more complex. Also Fonts are not heavily used.

* This source code is in the PUBLIC DOMAIN and has NO WARRANTY.
* by Waldo Alvarez Cañizares <wac at ghost.matcom.uh.cu>, started July 11, 2003. */

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "utils.h"

#define CONTROLCLASS  "COMBOBOX"  /* the class name */
#define CONTROLCLASSW L"COMBOBOX" /* the class name in unicode*/

#define WINDOWWIDTH 560
#define WINDOWHEIGHT 350

/* --- Command IDs of some buttons --- */
#define CREATEWINDOW_ID 106
#define CREATEWINDOWEX_ID 107
#define CREATEWINDOWW_ID 108
#define INITPAGE_ID 400
#define SECONDPAGE_ID 401
#define BACKFIRSTPAGE_ID 402

/* --- Position where the result text goes --- */
#define ResultX 0
#define ResultY 305

/* --- Position where the notify text goes --- */
#define NOTIFYX 390
#define NOTIFYY 285

/* --- The width of most buttons --- */
#define CHECKBUTWIDTH 190
#define SCROLLAMOUNT -15

/* Size of buffer to hold resulting strings from conversion
and returned by messages */
#define BUFFERLEN 80
char TextBuffer[BUFFERLEN]={'R','e','s','u','l','t',':',' '};

HWND g_hwnd = NULL;
HINSTANCE g_hInst = NULL;

int pos = 10;
int n = 0;
int yButPos = 10;
int xButPos = 0;

DWORD ComboStyle = 0;

/* --- Control coordinates --- */
#define CONTROLPOSX 390
#define CONTROLPOSY 10
DWORD ControlWidth = 160;
DWORD ControlHeight = 150;

static RECT  srect = {CONTROLPOSX,CONTROLPOSY,WINDOWWIDTH,WINDOWHEIGHT};

HWND hwndEdit = NULL;

RECT  rect;
DWORD StartP,EndP;
HWND hwnd; /* main window handle */

char AddString[] = "string added";

typedef void FunctionHandler(HWND,DWORD,WPARAM,LPARAM);
typedef FunctionHandler* LPFUNCTIONHANDLER;

void PrintTextXY(char* Text,int x,int y,int len, RECT rect)
    {
    HDC hdc;
    hdc = GetDC (g_hwnd);
    SelectObject (hdc, GetStockObject (SYSTEM_FIXED_FONT));

    TextOut (hdc, x,y,Text,len);
    ReleaseDC (g_hwnd, hdc);

    ValidateRect (g_hwnd, &rect);
    }

static
VOID
HandlePrintReturnHex(HWND handle,DWORD Msg,WPARAM wParam,LPARAM lParam)
    {
    LRESULT ret;
    RECT rect;
    ret = SendMessage(handle,Msg,wParam,lParam);
    htoa((unsigned int)ret,&TextBuffer[8]);
    GetWindowRect(g_hwnd,&rect);
    PrintTextXY(TextBuffer,ResultX,ResultY,16,rect);
    }


static
VOID
HandlePrintReturnStr(HWND handle,DWORD Msg,WPARAM wParam,LPARAM lParam)
    {
    LRESULT ret;
    RECT rect;

    TextBuffer[8] = (char)(BUFFERLEN - 8); /* Setting the max size to put chars in first byte */
    ret = SendMessage(handle,Msg,wParam,lParam);
    GetWindowRect(g_hwnd,&rect);
    PrintTextXY(TextBuffer,ResultX,ResultY,8+(int)ret,rect);
    }

static
VOID
HandlePrintRect(HWND handle,DWORD Msg,WPARAM wParam,LPARAM lParam)
    {
    RECT rect;
    TextBuffer[8] = (char)(BUFFERLEN - 8); /* Setting the max size to put chars in first byte */
    SendMessage(handle,Msg,wParam,lParam);

    htoa(rect.top,&TextBuffer[8]);
    TextBuffer[8+8] = ' ';
    htoa(rect.bottom,&TextBuffer[8+8+1]);
    TextBuffer[8+8+8+1] = ' ';
    htoa(rect.left,&TextBuffer[8+8+8+1+1]);
    TextBuffer[8+8+8+8+1+1] = ' ';
    htoa(rect.right,&TextBuffer[8+8+8+8+1+1+1]);

    GetWindowRect(g_hwnd,&rect);
    PrintTextXY(TextBuffer,ResultX,ResultY,8+4*9-1,rect);
    }

struct
    {
    char* Text;                	/* Text for the button */
    DWORD MsgCode;             	/* Message Code */
    WPARAM wParam;             	/* Well hope you can understand this */
    LPARAM lParam;             	/* ditto */
    LPFUNCTIONHANDLER Handler; 	/* Funtion called to handle the result of each message */
    }
Msg[] =
    {
		{"CB_ADDSTRING",CB_ADDSTRING,0,(LPARAM)&AddString,&HandlePrintReturnHex},
        {"CB_ADDSTRING - long",CB_ADDSTRING,0,(LPARAM)"very loooooooooong striiinnnnnnnnnggg",&HandlePrintReturnHex},
        {"CB_DELETESTRING",CB_DELETESTRING,2,0,&HandlePrintReturnHex},   /* remember to catch WM_DELETEITEM*/

        /* What a message, why M$ decided to implement his thing ? */
        {"CB_DIR - drives",CB_DIR,DDL_DRIVES,
        /* Hoping that most machines have this */
        (LPARAM)"C:\\",
        &HandlePrintReturnHex},

        {"CB_DIR - dirs",CB_DIR,DDL_DIRECTORY,(LPARAM)"C:\\*",&HandlePrintReturnHex},

        {"CB_DIR - files",CB_DIR,
        DDL_ARCHIVE | DDL_EXCLUSIVE | DDL_HIDDEN | DDL_READONLY | DDL_READWRITE | DDL_SYSTEM,
        (LPARAM)"C:\\*",&HandlePrintReturnHex},

        /* Do not forget WM_COMPAREITEM */

        {"CB_FINDSTRING",CB_FINDSTRING,1,(LPARAM)"str",&HandlePrintReturnHex},
        {"CB_FINDSTRINGEXACT(-1)",CB_FINDSTRINGEXACT,-1,(LPARAM)&AddString,&HandlePrintReturnHex},
        {"CB_FINDSTRINGEXACT(2)",CB_FINDSTRINGEXACT,2,(LPARAM)&AddString,&HandlePrintReturnHex},

        /* "CB_GETCOMBOBOXINFO",CB_GETCOMBOBOXINFO,0,0,&HandlePrintReturnHex, winXP & .net server remember to handle the struct  */

        {"CB_GETCOUNT",CB_GETCOUNT,0,0,&HandlePrintReturnHex},

        {"CB_GETCURSEL",CB_GETCURSEL,0,0,&HandlePrintReturnHex},

        /* To implement "CB_GETEDITSEL - vars",CB_GETEDITSEL,,,&HandlePrintReturnHex, */

        {"CB_GETEXTENDEDUI",CB_GETEXTENDEDUI,0,0,&HandlePrintReturnHex},
        {"CB_GETHORIZONTALEXTENT",CB_GETHORIZONTALEXTENT,0,0,&HandlePrintReturnHex},



        {"CB_GETLBTEXT",CB_GETLBTEXT,1,(LPARAM)&TextBuffer[8],&HandlePrintReturnStr},
        {"CB_GETLBTEXTLEN",CB_GETLBTEXTLEN,1,0,&HandlePrintReturnHex},
        {"CB_GETLOCALE",CB_GETLOCALE,0,0,&HandlePrintReturnHex},

        /* "CB_GETMINVISIBLE",CB_GETMINVISIBLE,0,0,&HandlePrintReturnHex, Included in Windows XP and Windows .NET Server. */

        {"CB_GETTOPINDEX",CB_GETTOPINDEX,0,0,&HandlePrintReturnHex},

        {"CB_INITSTORAGE",CB_INITSTORAGE,10,200,&HandlePrintReturnHex},
        {"CB_INSERTSTRING",CB_INSERTSTRING,2,(LPARAM)"inserted string",&HandlePrintReturnHex},

        {"CB_LIMITTEXT",CB_LIMITTEXT,10,0,&HandlePrintReturnHex},
        {"CB_RESETCONTENT",CB_RESETCONTENT ,0,0,&HandlePrintReturnHex},
        {"CB_SELECTSTRING",CB_SELECTSTRING,2,(LPARAM)"str",&HandlePrintReturnHex},
        {"CB_SETCURSEL",CB_SETCURSEL,1,0,&HandlePrintReturnHex},

        {"CB_SETDROPPEDWIDTH",CB_SETDROPPEDWIDTH,250,0,&HandlePrintReturnHex},

        {"CB_SETEXTENDEDUI - set",CB_SETEXTENDEDUI,TRUE,0,&HandlePrintReturnHex},
        {"CB_SETEXTENDEDUI - clear",CB_SETEXTENDEDUI,FALSE,0,&HandlePrintReturnHex},

        /*
        * win2k have a small bug with this ^ , if you press F4 while it is cleared,
        * the combobox is using style cbs_dropdown
        * and the pointer is over the edit box then the mouse pointer is not changed
        * to an arrow
        */

        {"CB_SETHORIZONTALEXTENT",CB_SETHORIZONTALEXTENT,500,0,&HandlePrintReturnHex},

        {"CB_GETITEMDATA",CB_GETITEMDATA,1,0,&HandlePrintReturnHex},
        {"CB_SETITEMDATA",CB_SETITEMDATA,1,0x791031,&HandlePrintReturnHex},

        {"CB_SETITEMHEIGHT",CB_SETITEMHEIGHT,-1,30,&HandlePrintReturnHex},
        {"CB_GETITEMHEIGHT",CB_GETITEMHEIGHT,2,0,&HandlePrintReturnHex},

        /* "CB_SETMINVISIBLE",CB_SETMINVISIBLE,4,0,&HandlePrintReturnHex, Included in Windows XP and Windows .NET Server */

        {"CB_GETEDITSEL",CB_GETEDITSEL,(WPARAM)NULL,(LPARAM)NULL,&HandlePrintReturnHex},
        {"CB_SETEDITSEL",CB_SETEDITSEL,0,0x00020005,&HandlePrintReturnHex},
        {"CB_SETEDITSEL - clear",CB_SETEDITSEL,0,0xFFFFFFFF,&HandlePrintReturnHex},

        {"CB_SETTOPINDEX",CB_SETTOPINDEX,3,0,&HandlePrintReturnHex},

        {"CB_SHOWDROPDOWN - true",CB_SHOWDROPDOWN,TRUE,0,&HandlePrintReturnHex},
        {"CB_SHOWDROPDOWN - false",CB_SHOWDROPDOWN,FALSE,0,&HandlePrintReturnHex},

        {"CB_GETDROPPEDCONTROLRECT",CB_GETDROPPEDCONTROLRECT,0,(LPARAM)&rect,&HandlePrintRect},
        {"CB_GETDROPPEDSTATE",CB_GETDROPPEDSTATE,0,0,&HandlePrintReturnHex},
        {"CB_GETDROPPEDWIDTH",CB_GETDROPPEDWIDTH,0,0,&HandlePrintReturnHex},

        {"WM_PASTE",WM_PASTE,0,0,&HandlePrintReturnHex},
    };

#define MAXMESSAGEBUTTONS 40

struct
    {
    char* Name;                	/* Text for the button */
    DWORD Code;             	/* Style Code */
    }
Styles[] = {
    {"WS_DISABLED",WS_DISABLED},
	{"CBS_AUTOHSCROLL",CBS_AUTOHSCROLL},
	{"CBS_DISABLENOSCROLL",CBS_DISABLENOSCROLL},
	{"CBS_DROPDOWN",CBS_DROPDOWN},
	{"CBS_DROPDOWNLIST",CBS_DROPDOWNLIST},
	{"CBS_HASSTRINGS",CBS_HASSTRINGS},
	{"CBS_LOWERCASE",CBS_LOWERCASE},
	{"CBS_NOINTEGRALHEIGHT",CBS_NOINTEGRALHEIGHT},
	{"CBS_OEMCONVERT",CBS_OEMCONVERT},
	{"CBS_OWNERDRAWFIXED",CBS_OWNERDRAWFIXED},
	{"CBS_OWNERDRAWVARIABLE",CBS_OWNERDRAWVARIABLE},
	{"CBS_SIMPLE",CBS_SIMPLE},
	{"CBS_SORT",CBS_SORT},
	{"CBS_UPPERCASE",CBS_UPPERCASE},
	{"CBS_DISABLENOSCROLL",CBS_DISABLENOSCROLL},
	{"WS_HSCROLL",WS_HSCROLL},
	{"WS_VSCROLL",WS_VSCROLL}
    };

/* The number of check buttons we have.
* Maybe some calculations at compile time would be better
*/

#define NUMBERCHECKS 17

#define NUMBERBUTTONS  NUMBERCHECKS + 7
HWND Buttons[NUMBERBUTTONS];
HWND MessageButtons[MAXMESSAGEBUTTONS];
HWND Back1But,Back2But;
HWND NextBut;

HWND
CreateCheckButton(const char* lpWindowName, DWORD xSize, DWORD id)
    {
    HWND h;
    h  = CreateWindowEx(0,
        "BUTTON",
        lpWindowName,
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        xButPos, /*  x  */
        yButPos, /*  y  */
        xSize,   /* nWidth  */
        20,      /* nHeight */
        g_hwnd,
        (HMENU) id,
        g_hInst,
        NULL
        );
    yButPos += 21;
    return h;
    }

HWND
CreatePushButton(const char* lpWindowName, DWORD xSize, DWORD id,DWORD Style)
    {

    HWND h = CreateWindow("BUTTON",
        lpWindowName,
        WS_CHILD | BS_PUSHBUTTON | Style,
        xButPos, /* x */
        yButPos, /* y */
        xSize,   /* nWidth */
        20,      /* nHeight */
        g_hwnd,
        (HMENU) id,
        g_hInst,
        NULL
        );

    yButPos += 21;
    return h;
    }

VOID
ReadNHide()
    {
    int i;
    ComboStyle = 0;
    for (i=0 ; i< NUMBERCHECKS ; i++)
        {
        if(BST_CHECKED == SendMessage(Buttons[i],BM_GETCHECK,0,0))
            ComboStyle |= Styles[i].Code;
        ShowWindow(Buttons[i],SW_HIDE);
        }

    for (; i< NUMBERBUTTONS ; i++)ShowWindow(Buttons[i],SW_HIDE);
    for (i=0 ; i< 26 ; i++) ShowWindow(MessageButtons[i],SW_SHOW);

    ShowWindow(Back1But,SW_SHOW);
    ShowWindow(NextBut,SW_SHOW);
    }

VOID
ForwardToSecondPage()
    {
    int i;
    for (i=0;i<26;i++)ShowWindow(MessageButtons[i],SW_HIDE);
    for(;i<MAXMESSAGEBUTTONS;i++)ShowWindow(MessageButtons[i],SW_SHOW);
    ShowWindow(Back2But,SW_SHOW);

    ShowWindow(Back1But,SW_HIDE);
    ShowWindow(NextBut,SW_HIDE);
    }

VOID
BackToFirstPage()
    {
    int i;
    for (i=0;i<26;i++)ShowWindow(MessageButtons[i],SW_SHOW);
    for(;i<MAXMESSAGEBUTTONS;i++)ShowWindow(MessageButtons[i],SW_HIDE);
    ShowWindow(Back2But,SW_HIDE);
    ShowWindow(Back1But,SW_SHOW);
    ShowWindow(NextBut,SW_SHOW);
    }

VOID
BackToInitialPage()
    {
    int i;
    DestroyWindow(hwndEdit);
    for (i=0 ; i< NUMBERBUTTONS ; i++) {ShowWindow(Buttons[i],SW_SHOW);}
for (i=0;i<26;i++)ShowWindow(MessageButtons[i],SW_HIDE);
ShowWindow(Back1But,SW_HIDE);
ShowWindow(NextBut,SW_HIDE);
    }

LRESULT
CALLBACK
WndProc ( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
    {
    int i;
    switch ( msg )
        {
        case WM_CREATE:
            g_hwnd = hwnd;

            /* ---- Initial page ---- */

            for (i = 0 ; i < 14 ; i++)
                Buttons[i] = CreateCheckButton(Styles[i].Name,CHECKBUTWIDTH,500+i);

            xButPos += CHECKBUTWIDTH + 10;
            yButPos = 10;

            for (; i < NUMBERCHECKS ; i++)
                Buttons[i] = CreateCheckButton(Styles[i].Name,CHECKBUTWIDTH,500+i);

            Buttons[i++] = CreatePushButton("Width +",70,100,WS_VISIBLE);
            Buttons[i++] = CreatePushButton("Width -",70,101,WS_VISIBLE);

            Buttons[i++] = CreatePushButton("Heigth +",70,102,WS_VISIBLE);
            Buttons[i++] = CreatePushButton("Heigth -",70,103,WS_VISIBLE);

            Buttons[i++] = CreatePushButton("CreateWindowA",CHECKBUTWIDTH,CREATEWINDOW_ID,WS_VISIBLE);
            Buttons[i++] = CreatePushButton("CreateWindowExA",CHECKBUTWIDTH,CREATEWINDOWEX_ID,WS_VISIBLE);
            Buttons[i++] = CreatePushButton("CreateWindowExW",CHECKBUTWIDTH,CREATEWINDOWW_ID,WS_VISIBLE);


            /* ---- The 1st page of buttons ---- */

            xButPos = 0;
            yButPos = 10;

            for (i = 0 ; i < 14 ; i++)
                MessageButtons[i] = CreatePushButton(Msg[i].Text,CHECKBUTWIDTH,600+i,0);

            xButPos += CHECKBUTWIDTH + 10;
            yButPos = 10;

            for (; i < 26 ; i++)
                MessageButtons[i] = CreatePushButton(Msg[i].Text,CHECKBUTWIDTH,600+i,0);

            Back1But = CreatePushButton("Back - destroys ComboBox",CHECKBUTWIDTH,INITPAGE_ID,0);
            NextBut = CreatePushButton("Next",CHECKBUTWIDTH,SECONDPAGE_ID,0);

            /* ---- The 2nd page of buttons ------*/

            xButPos = 0;
            yButPos = 10;

            for (; i<40; i++)
                MessageButtons[i] = CreatePushButton(Msg[i].Text,CHECKBUTWIDTH,600+i,0);

            xButPos += CHECKBUTWIDTH + 10;
            yButPos = 10;

            for (; i < MAXMESSAGEBUTTONS ; i++)
                MessageButtons[i] = CreatePushButton(Msg[i].Text,CHECKBUTWIDTH,600+i,0);

            Back2But = CreatePushButton("Back",CHECKBUTWIDTH,BACKFIRSTPAGE_ID,0);

            break;

        case WM_COMMAND:
            if (LOWORD(wParam) >= 600)
                {
                Msg[LOWORD(wParam)-600].Handler(hwndEdit,
                    Msg[LOWORD(wParam)-600].MsgCode,
                    Msg[LOWORD(wParam)-600].wParam,
                    Msg[LOWORD(wParam)-600].lParam);
                break;
                }

            switch(LOWORD(wParam)){

        case 100:
            ControlWidth += 10;
            break;

        case 101:
            ControlWidth -= 10;
            break;

        case 102:
            ControlHeight += 10;
            break;

        case 103:
            ControlHeight -= 10;
            break;

        case INITPAGE_ID:
            BackToInitialPage();
            break;

        case SECONDPAGE_ID:
            ForwardToSecondPage();
            break;

        case BACKFIRSTPAGE_ID:
            BackToFirstPage();
            break;

        case CREATEWINDOW_ID:
            ReadNHide();
            srect.top = CONTROLPOSY + ControlHeight;
            hwndEdit = CreateWindow(CONTROLCLASS,
                NULL,
                ComboStyle | WS_CHILD | WS_VISIBLE,
                CONTROLPOSX,
                CONTROLPOSY,
                ControlWidth,
                ControlHeight,
                g_hwnd,
                NULL,
                g_hInst,
                NULL);
            break;

        case CREATEWINDOWEX_ID:
            ReadNHide();
            srect.top = CONTROLPOSY + ControlHeight;
            hwndEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
                CONTROLCLASS,
                NULL,
                ComboStyle | WS_CHILD | WS_VISIBLE ,
                CONTROLPOSX,
                CONTROLPOSY,
                ControlWidth,
                ControlHeight,
                g_hwnd,
                NULL,
                g_hInst,
                NULL);
            break;

        case CREATEWINDOWW_ID:
            ReadNHide();
            srect.top = CONTROLPOSY + ControlHeight;
            hwndEdit = CreateWindowExW(WS_EX_CLIENTEDGE,
                CONTROLCLASSW,
                NULL,
                ComboStyle | WS_CHILD | WS_VISIBLE ,
                CONTROLPOSX,
                CONTROLPOSY,
                ControlWidth,
                ControlHeight,
                g_hwnd,
                NULL,
                g_hInst,
                NULL);
            break;
                }

        if (lParam == (LPARAM)hwndEdit)
            switch(HIWORD(wParam))
            {
                case CBN_DROPDOWN:
                    ScrollWindow (hwnd, 0, SCROLLAMOUNT, &srect, &srect);
                    PrintTextXY("CBN_DROPDOWN notification",NOTIFYX,NOTIFYY,25,srect);
                    break;

                case CBN_CLOSEUP:
                    ScrollWindow (hwnd, 0, SCROLLAMOUNT, &srect, &srect);
                    PrintTextXY("CBN_CLOSEUP notification",NOTIFYX,NOTIFYY,24,srect);
                    break;

                case CBN_DBLCLK:
                    ScrollWindow (hwnd, 0, SCROLLAMOUNT, &srect, &srect);
                    PrintTextXY("CBN_DBLCLK notification",NOTIFYX,NOTIFYY,23,srect);
                    break;

                case CBN_EDITCHANGE:
                    ScrollWindow (hwnd, 0, SCROLLAMOUNT, &srect, &srect);
                    PrintTextXY("CBN_EDITCHANGE notification",NOTIFYX,NOTIFYY,27,srect);
                    break;

                case (WORD)CBN_ERRSPACE:
                    ScrollWindow (hwnd, 0, SCROLLAMOUNT, &srect, &srect);
                    PrintTextXY("CBN_ERRSPACE notification",NOTIFYX,NOTIFYY,25,srect);
                    break;

                case CBN_KILLFOCUS:
                    ScrollWindow (hwnd, 0, SCROLLAMOUNT, &srect, &srect);
                    PrintTextXY("CBN_KILLFOCUS notification",NOTIFYX,NOTIFYY,26,srect);
                    break;

                case CBN_EDITUPDATE:
                    ScrollWindow (hwnd, 0, SCROLLAMOUNT, &srect, &srect);
                    PrintTextXY("CBN_EDITUPDATE notification",NOTIFYX,NOTIFYY,27,srect);
                    break;

                case CBN_SELCHANGE:
                    ScrollWindow (hwnd, 0, SCROLLAMOUNT, &srect, &srect);
                    PrintTextXY("CBN_SELCHANGE notification",NOTIFYX,NOTIFYY,26,srect);
                    break;

                case CBN_SELENDCANCEL:
                    ScrollWindow (hwnd, 0, SCROLLAMOUNT, &srect, &srect);
                    PrintTextXY("CBN_SELENDCANCEL notification",NOTIFYX,NOTIFYY,29,srect);
                    break;

                case CBN_SETFOCUS:
                    ScrollWindow (hwnd, 0, SCROLLAMOUNT, &srect, &srect);
                    PrintTextXY("CBN_SETFOCUS notification",NOTIFYX,NOTIFYY,25,srect);
                    break;

                case CBN_SELENDOK:
                    ScrollWindow (hwnd, 0, SCROLLAMOUNT, &srect, &srect);
                    PrintTextXY("CBN_SELENDOK notification",NOTIFYX,NOTIFYY,25,srect);
                    break;
            }

        return DefWindowProc ( hwnd, msg, wParam, lParam );

        case WM_MEASUREITEM:
            ScrollWindow (hwnd, 0, SCROLLAMOUNT, &srect, &srect);
            PrintTextXY("WM_MEASUREITEM called",NOTIFYX,NOTIFYY,21,srect);
            break;

        case WM_COMPAREITEM:
            ScrollWindow (hwnd, 0, SCROLLAMOUNT, &srect, &srect);
            PrintTextXY("WM_COMPAREITEM called",NOTIFYX,NOTIFYY,21,srect);
            break;

        case WM_DRAWITEM:
            ScrollWindow (hwnd, 0, SCROLLAMOUNT, &srect, &srect);
            PrintTextXY("WM_DRAWITEM called",NOTIFYX,NOTIFYY,18,srect);
            break;

        case WM_SIZE :
            return 0;

        case WM_CLOSE:
            DestroyWindow (g_hwnd);
            return 0;

        case WM_QUERYENDSESSION:
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
    return DefWindowProc ( hwnd, msg, wParam, lParam );
    }


HWND
RegisterAndCreateWindow (HINSTANCE hInst,
                         const char* className,
                         const char* title)
    {
    WNDCLASSEX wc;


    g_hInst = hInst;

    wc.cbSize = sizeof (WNDCLASSEX);

    wc.lpfnWndProc = WndProc;   /* window procedure */
    wc.hInstance = hInst;       /* owner of the class */

    wc.lpszClassName = className;
    wc.hCursor = LoadCursor ( 0, (LPCTSTR)IDC_ARROW );
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hIcon = 0;
    wc.hIconSm = 0;
    wc.lpszMenuName = 0;

    if ( !RegisterClassEx ( &wc ) )
        return NULL;

    hwnd = CreateWindowEx (
        0,          /* dwStyleEx */
        className,  /* class name */
        title,      /* window title */

        WS_OVERLAPPEDWINDOW, /* dwStyle */

        1,            /* x */
        1,            /* y */
        WINDOWWIDTH,  /* width */
        WINDOWHEIGHT, /* height */
        NULL,         /* hwndParent */
        NULL,         /* hMenu */
        hInst,
        0
        );

    if (!hwnd) return NULL;

    ShowWindow (hwnd, SW_SHOW);
    UpdateWindow (hwnd);

    return hwnd;
    }

int
WINAPI
WinMain ( HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdParam, int cmdShow )
    {
    char className [] = "ComboBox Control Test";
    MSG msg;

    RegisterAndCreateWindow ( hInst, className, "ComboBox Control Test" );

    while (GetMessage (&msg, NULL, 0, 0))
        {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
        }
    return (int)msg.wParam;
    }
