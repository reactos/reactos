/* Edit Control Test for ReactOS, quick n' dirty. Very rigid too.
 * There you go, is only a test program. Not made to be fast, small
 * easy to mantain, or portable. Lots of duplicated code too.

 * I'm not erasing text because I don't want to use other functions from th API
 * or make this more complex.

 * This source code is in the PUBLIC DOMAIN and has NO WARRANTY.
 * by Waldo Alvarez Cañizares <wac at ghost.matcom.uh.cu>, June 22, 2003. */

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "utils.h"

#define CREATEWINDOW 106
#define CREATEWINDOWEX 107
#define CREATEWINDOWW 108

#define ResultX 0
#define ResultY 305

#define NOTIFYX 350
#define NOTIFYY 285

#define BUFFERLEN 80 /* Size of buffer to hold result strings */

/* Edit is created with this text */
#define TestStr "The quick brown fox jumps over the lazy dog" 

#define TestStrW L"This is a WCHAR string" /*  Wide to support unicode edits */

#define MAXMESSAGEBUTTONS 42

HWND g_hwnd = NULL;
HINSTANCE g_hInst = NULL;

int pos = 10;
int n = 0;
int yButPos = 10;
int xButPos = 10;

DWORD EditStyle = 0;
DWORD EditWidth = 240;
DWORD EditHeight = 250;

BOOL UnicodeUsed = FALSE;

HWND hwndEdit = NULL;

POINTL point={10,3};
RECT  rect = {0,0,20,20},rect2;
DWORD StartP,EndP;

#define ReplaceText "->> Replaced!! <<-"

char* AllocatedText;  /* Buffer in the heap to feed it to the edit control */
char* NewText = "New text for the edit control";
wchar_t* NewTextW = L"New text for the edit control in UNICODE"; // Wide

char TextBuffer[BUFFERLEN]={'R','e','s','u','l','t',':',' '};

typedef void FunctionHandler(HWND,DWORD,WPARAM,LPARAM);
typedef FunctionHandler* LPFUNCTIONHANDLER;

PrintTextXY(char* Text,int x,int y,int len)
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
      int ret;
      ret = SendMessage(handle,Msg,wParam,lParam);
      htoa(ret,&TextBuffer[8]);
      PrintTextXY(TextBuffer,ResultX,ResultY,16);
    }

static
VOID
HandleSetHandlePrintHex(HWND handle,DWORD Msg,WPARAM wParam,LPARAM lParam)
    {
      LPVOID pMem;
      HANDLE hNewBuffer;
      int ret;

      LocalFree((HLOCAL)SendMessage(handle, EM_GETHANDLE, 0, 0L));
      if (UnicodeUsed)
          {
            hNewBuffer = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, 100);
            pMem = LocalLock(hNewBuffer);
            strcpyw_((wchar_t*)pMem,NewTextW);
          }
      else
          {
            hNewBuffer = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,50);
            pMem = LocalLock(hNewBuffer);
            strcpy_((char*)pMem,NewText);
          }

      LocalUnlock(pMem);
      hNewBuffer = LocalHandle(pMem);

	  /* Updates the buffer and displays new buffer */
      ret =  SendMessage(handle, EM_SETHANDLE, (WPARAM)hNewBuffer, 0L);

      htoa(ret,&TextBuffer[8]);
      PrintTextXY(TextBuffer,ResultX,ResultY,16);
    }

static
VOID
HandlePrintReturnStr(HWND handle,DWORD Msg,WPARAM wParam,LPARAM lParam)
    {
      int ret;
      TextBuffer[8] = (char)(BUFFERLEN - 8); /* Setting the max size to put chars in first byte */
      ret = SendMessage(handle,Msg,wParam,lParam);
      PrintTextXY(TextBuffer,ResultX,ResultY,8+ret);
    }

static
VOID
HandlePrintRect(HWND handle,DWORD Msg,WPARAM wParam,LPARAM lParam)
    {
      TextBuffer[8] = (char)(BUFFERLEN - 8); /* Setting the max size to put chars in first byte */
      SendMessage(handle,Msg,wParam,lParam);

      htoa(rect.top,&TextBuffer[8]);
      TextBuffer[8+8] = ' ';
      htoa(rect.bottom,&TextBuffer[8+8+1]);
      TextBuffer[8+8+8+1] = ' ';
      htoa(rect.left,&TextBuffer[8+8+8+1+1]);
      TextBuffer[8+8+8+8+1+1] = ' ';
      htoa(rect.right,&TextBuffer[8+8+8+8+1+1+1]);

      PrintTextXY(TextBuffer,ResultX,ResultY,8+4*9-1);
    }

static
VOID
HandlePrintPasswdChar(HWND handle,DWORD Msg,WPARAM wParam,LPARAM lParam)
    {
      HDC hdc; 
      int ret = SendMessage(handle,Msg,wParam,lParam);

      int s;

      if (ret)
          {
            s = 1;
            TextBuffer[8] = (char)(ret);
          }
      else
          {
            TextBuffer[8]  = 'N';
            TextBuffer[9]  = 'U';
            TextBuffer[10] = 'L';
            TextBuffer[11] = 'L';
            s = 4;
          }

      hdc = GetDC (g_hwnd);
      SelectObject (hdc, GetStockObject (SYSTEM_FIXED_FONT));

      TextOut (hdc,ResultX ,ResultY,TextBuffer,8+s);
      ReleaseDC (g_hwnd, hdc);
      ValidateRect (g_hwnd, &rect);
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
     "EM_CANUNDO",EM_CANUNDO,0,0,&HandlePrintReturnHex,
     "EM_CHARFROMPOS",EM_CHARFROMPOS,(WPARAM)&point,0,&HandlePrintReturnHex,
     "EM_EMPTYUNDOBUFFER",EM_EMPTYUNDOBUFFER,0,0,&HandlePrintReturnHex,
     "EM_FMTLINES",EM_FMTLINES,TRUE,0,&HandlePrintReturnHex,
     "EM_GETFIRSTVISIBLELINE",EM_GETFIRSTVISIBLELINE,0,0,&HandlePrintReturnHex,

     "EM_GETLIMITTEXT",EM_GETLIMITTEXT,0,0,&HandlePrintReturnHex,
     "EM_GETLINE",EM_GETLINE,2,(WPARAM)&TextBuffer[8],&HandlePrintReturnStr,
     "EM_GETLINECOUNT",EM_GETLINECOUNT,0,0,&HandlePrintReturnHex,
     "EM_GETMARGINS",EM_GETMARGINS,0,0,&HandlePrintReturnHex,
     "EM_SETMARGINS",EM_SETMARGINS,EC_LEFTMARGIN,10,&HandlePrintReturnHex,

     "EM_GETMODIFY",EM_GETMODIFY,0,0,&HandlePrintReturnHex,
     "EM_SETMODIFY",EM_SETMODIFY,TRUE,0,&HandlePrintReturnHex,
     
     "EM_GETSEL",EM_GETSEL,(WPARAM)&StartP,(LPARAM)&EndP,&HandlePrintReturnHex,

     "EM_GETTHUMB",EM_GETTHUMB,0,0,&HandlePrintReturnHex,
     
     "EM_LIMITTEXT",EM_LIMITTEXT,10,0,&HandlePrintReturnHex,
     "EM_LINEFROMCHAR",EM_LINEFROMCHAR,-1,0,&HandlePrintReturnHex,
     "EM_POSFROMCHAR",EM_POSFROMCHAR,10,0,&HandlePrintReturnHex,
     "EM_LINEINDEX",EM_LINEINDEX,2,0,&HandlePrintReturnHex,
     "EM_LINELENGTH",EM_LINELENGTH,-1,0,&HandlePrintReturnHex,

     "EM_GETWORDBREAKPROC",EM_GETWORDBREAKPROC,0,0,&HandlePrintReturnHex,
     "EM_REPLACESEL",EM_REPLACESEL,TRUE,(LPARAM)&ReplaceText,&HandlePrintReturnHex,

     "EM_LINESCROLL",EM_LINESCROLL,5,1,&HandlePrintReturnHex,
     "EM_SCROLL",EM_SCROLL,SB_LINEDOWN,0,&HandlePrintReturnHex,
     "EM_SCROLLCARET",EM_SCROLLCARET,0,0,&HandlePrintReturnHex,

     "EM_SETHANDLE",EM_SETHANDLE,0,0,&HandleSetHandlePrintHex,
     "EM_GETHANDLE",EM_GETHANDLE,0,0,&HandlePrintReturnHex,
     "EM_GETPASSWORDCHAR",EM_GETPASSWORDCHAR,0,0,&HandlePrintPasswdChar,
     "EM_SETPASSWORDCHAR - clear",EM_SETPASSWORDCHAR,0,0,&HandlePrintReturnHex,
     "EM_SETPASSWORDCHAR - x",EM_SETPASSWORDCHAR,'x',0,&HandlePrintReturnHex,

     "EM_SETREADONLY - set",EM_SETREADONLY,TRUE,0,&HandlePrintReturnHex,
     "EM_SETREADONLY - clear",EM_SETREADONLY,FALSE,0,&HandlePrintReturnHex,

     "EM_GETRECT",EM_GETRECT,0,(LPARAM)&rect2,&HandlePrintRect,
     "EM_SETRECT",EM_SETRECT,0,(LPARAM)&rect,&HandlePrintReturnHex,
     "EM_SETRECTNP",EM_SETRECTNP,0,(LPARAM)&rect,&HandlePrintReturnHex,
     "EM_SETSEL",EM_SETSEL,1,3,&HandlePrintReturnHex,

     "EM_SETSEL - all",EM_SETSEL,0,-1,&HandlePrintReturnHex,
     "EM_SETSEL - remove",EM_SETSEL,-1,0,&HandlePrintReturnHex,
     "EM_UNDO",EM_UNDO,0,0,&HandlePrintReturnHex,
     "WM_UNDO",WM_UNDO,0,0,&HandlePrintReturnHex,
     "WM_PASTE",WM_PASTE,0,0,&HandlePrintReturnHex,

     "WM_CUT",WM_CUT,0,0,&HandlePrintReturnHex,
     "WM_COPY",WM_COPY,0,0,&HandlePrintReturnHex
     
};

DWORD EditStyles[] = {
                      WS_THICKFRAME,WS_DISABLED,WS_BORDER,ES_LOWERCASE,ES_UPPERCASE,ES_NUMBER,ES_AUTOVSCROLL,
                      ES_AUTOHSCROLL,ES_LEFT,ES_CENTER,ES_RIGHT,ES_MULTILINE,
                      ES_NOHIDESEL,ES_OEMCONVERT,ES_PASSWORD,ES_READONLY,ES_WANTRETURN,
                      WS_HSCROLL,WS_VSCROLL
                     };

char* StyleNames[] = {
       		      "WS_THICKFRAME","WS_DISABLED","WS_BORDER","ES_LOWERCASE","ES_UPPERCASE","ES_NUMBER","ES_AUTOVSCROLL",
                      "ES_AUTOHSCROLL","ES_LEFT","ES_CENTER","ES_RIGHT","ES_MULTILINE",
                      "ES_NOHIDESEL","ES_OEMCONVERT","ES_PASSWORD","ES_READONLY","ES_WANTRETURN",
                      "WS_HSCROLL","WS_VSCROLL"
                     };

#define NUMBERBUTTONS 26
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
					      xButPos, // x
		                  yButPos, // y
		                  xSize,   // nWidth
		                  20,      // nHeight
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
        EditStyle = 0;
        for (i=0 ; i< 19 ; i++)
        {
        if(BST_CHECKED == SendMessage(Buttons[i],BM_GETCHECK,0,0))
        EditStyle |= EditStyles[i];
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
            Buttons[i] = CreateCheckButton(StyleNames[i],150,500+i);

        xButPos += 160;
        yButPos = 10;

        for (; i < 19 ; i++)
            Buttons[i] = CreateCheckButton(StyleNames[i],140,500+i);

        Buttons[i++] = CreatePushButton("Width +",70,100,WS_VISIBLE);
        Buttons[i++] = CreatePushButton("Width -",70,101,WS_VISIBLE);

        Buttons[i++] = CreatePushButton("Heigth +",70,102,WS_VISIBLE);
        Buttons[i++] = CreatePushButton("Heigth -",70,103,WS_VISIBLE);

        Buttons[i++] = CreatePushButton("CreateWindowA",140,CREATEWINDOW,WS_VISIBLE);
        Buttons[i++] = CreatePushButton("CreateWindowExA",140,CREATEWINDOWEX,WS_VISIBLE);
        Buttons[i++] = CreatePushButton("CreateWindowExW",140,CREATEWINDOWW,WS_VISIBLE);
        

        /* ---- The 1st page of buttons ---- */

        xButPos = 0;
        yButPos = 10;

        for (i = 0 ; i < 14 ; i++)
            MessageButtons[i] = CreatePushButton(Msg[i].Text,170,600+i,0);

        xButPos += 180;
        yButPos = 10;

        for (; i < 26 ; i++)
            MessageButtons[i] = CreatePushButton(Msg[i].Text,170,600+i,0);

        Back1But = CreatePushButton("Back - destroys edit",170,400,0);
        NextBut = CreatePushButton("Next",170,401,0);

        /* ---- The 2nd page of buttons ------*/

        xButPos = 0;
        yButPos = 10;

        for (; i<40; i++)
            MessageButtons[i] = CreatePushButton(Msg[i].Text,170,600+i,0);

        xButPos += 180;
        yButPos = 10;

        for (; i < MAXMESSAGEBUTTONS ; i++)
            MessageButtons[i] = CreatePushButton(Msg[i].Text,170,600+i,0);

        Back2But = CreatePushButton("Back",170,402,0);

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
            EditWidth += 10;
            break;

        case 101:
            EditWidth -= 10;
            break;

        case 102:
            EditHeight += 10;
            break;

        case 103:
            EditHeight -= 10;
            break;

        case 400:
            BackToInitialPage();
            break;

        case 401:
            ForwardToSecondPage();
            break;

        case 402:
            BackToFirstPage();
            break;

        case CREATEWINDOW:
                  UnicodeUsed = FALSE;
                  ReadNHide();
                  hwndEdit = CreateWindow("EDIT",
                               TestStr,
                               EditStyle | WS_CHILD | WS_VISIBLE,
                               350,
                               10,
                               EditWidth,
                               EditHeight,
                               g_hwnd,
                               NULL,
                               g_hInst,
                               NULL);
                  break;

        case CREATEWINDOWEX:
                  UnicodeUsed = FALSE;
                  ReadNHide();
                  hwndEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
                               "EDIT",
                               TestStr,
                               EditStyle | WS_CHILD | WS_VISIBLE ,
                               350,
                               10,
                               EditWidth,
                               EditHeight,
                               g_hwnd,
                               NULL,
                               g_hInst,
                               NULL);
                  break;

        case CREATEWINDOWW:
                  UnicodeUsed = TRUE;
                  ReadNHide();
                  hwndEdit = CreateWindowExW(WS_EX_CLIENTEDGE,
                               L"EDIT",
                               TestStrW,
                               EditStyle | WS_CHILD | WS_VISIBLE ,
                               350,
                               10,
                               EditWidth,
                               EditHeight,
                               g_hwnd,
                               NULL,
                               g_hInst,
                               NULL);
                  break;
            }

	if (lParam == (LPARAM)hwndEdit)
		switch(HIWORD(wParam))
		{
		case EN_CHANGE:
                PrintTextXY("EN_CHANGE notification",NOTIFYX,NOTIFYY,22);
		break;

                case EN_ERRSPACE:
		PrintTextXY("EN_ERRSPACE notification",NOTIFYX,NOTIFYY,24);
		break;

		/* --- FIXME not defined in w32api-2.3 headers
		case H_SCROLL:
		PrintTextXY("H_SCROLL notification",NOTIFYX,NOTIFYY,21);
		break; */

		/* --- FIXME not defined in w32api-2.3 headers
		case KILL_FOCUS:
		PrintTextXY("KILL_FOCUS notification",NOTIFYX,NOTIFYY,23);
		break; */

		/* --- FIXME not defined in w32api-2.3 headers
		case EN_MAXTEST:
		PrintTextXY("EN_MAXTEXT notification",NOTIFYX,NOTIFYY,23);
		break; */

		case EN_SETFOCUS:
		PrintTextXY("EN_SETFOCUS notification",NOTIFYX,NOTIFYY,24);
		break;

		case EN_UPDATE:
		PrintTextXY("EN_UPDATE notification",NOTIFYX,NOTIFYY + 20,22);
		break;

		case EN_VSCROLL:
		PrintTextXY("EN_VSCROLL notification",NOTIFYX,NOTIFYY,23);
		break;

		}

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
	HWND hwnd;

	g_hInst = hInst;

	wc.cbSize = sizeof (WNDCLASSEX);

	wc.lpfnWndProc = WndProc;   /* window procedure */
	wc.hInstance = hInst;       /* owner of the class */

	wc.lpszClassName = className; 
	wc.hCursor = LoadCursor ( 0, IDC_ARROW );
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
				0, 	   /* dwStyleEx */
				className, /* class name */
				title,     /* window title */

				WS_OVERLAPPEDWINDOW, /* dwStyle */

				1,    /* x */
				1,    /* y */
				560,  /* width */
				350,  /* height */
				NULL, /* hwndParent */
				NULL, /* hMenu */
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
	char className [] = "Edit Control Test";
	MSG msg;

	RegisterAndCreateWindow ( hInst, className, "Edit Control Styles Test" );

	// Message loop
	while (GetMessage (&msg, NULL, 0, 0))
     {
          TranslateMessage (&msg);
          DispatchMessage (&msg);
     }
     return msg.wParam;

}
