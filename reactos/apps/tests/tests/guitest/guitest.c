/* Windows GUI Behaviour Tester */
/* by Ove Kåven */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>

#include "guitest.rc"

/* checks to include */
#define LOGGING /* can be undefined under Wine and use -debugmsg +message instead */
#define MAIN_STYLE WS_OVERLAPPEDWINDOW|WS_HSCROLL
#define MAIN_EXSTYLE 0
#undef TEST_DESTROY_MAIN
#define SHOW_SUB
#undef TEST_DIALOG
#define RESIZE_DIALOG
#undef TEST_SUBDIALOG
#undef TEST_COMMCTL

/************************/
/*** GLOBAL VARIABLES ***/
/************************/

HINSTANCE hInst;
DWORD StartTime;
HWND hListBox,hMainWnd,hSubWnd;
HWND hButton[4]={0,0,0,0};
HWND hDialog=0,hGroup=0,hSubDlg=0;
WNDPROC wndButton[4],wndDialog,wndGroup,wndSubDlg;
BOOL Clicked=0,Ready=0;
int State=0,Rec=0;
#define STATE_CREATE 0
#define STATE_DESTROY 1
#define STATE_SHOW 2
#define STATE_UPDATE 3
#define STATE_DIALOG 4
#define STATE_TEST 5
#define STATE_DIRECT 6
#define STATE_DISPATCH 7
#define STATE_RECURS 8
char*StateName[]={
 "Creat",
 "Destr",
 "Show ",
 "Updat",
 "Dialg",
 "Test ",
 "Call ",
 "Disp ",
 "RCall"
};

static char wclassname[] = "GUITestClass";
static char wcclassname[] = "GUITestChildClass";
static char winname[] = "GUITest";

/**************************/
/*** LOGGING FACILITIES ***/
/**************************/

struct MSGNAMES {
 int msg;
 char*name;
} MsgNames[]={
#define MSG(x) {x,#x},
#define MSG2(x,y) {y,#x},
#define ENDMSG {0}

/* we get these in CreateWindow */
MSG(WM_GETMINMAXINFO)
MSG(WM_NCCREATE)
MSG(WM_NCCALCSIZE)
MSG(WM_CREATE)
MSG(WM_PARENTNOTIFY)

/* we get these in ShowWindow */
MSG(WM_SHOWWINDOW)
MSG(WM_WINDOWPOSCHANGING)
MSG(WM_QUERYNEWPALETTE)
MSG(WM_ACTIVATEAPP)
MSG(WM_NCACTIVATE)
MSG(WM_GETTEXT)
MSG(WM_ACTIVATE)
MSG(WM_SETFOCUS)
MSG(WM_NCPAINT)
MSG(WM_ERASEBKGND)
MSG(WM_WINDOWPOSCHANGED)
MSG(WM_SIZE)
MSG(WM_MOVE)

/* we get these in DestroyWindow */
MSG(WM_KILLFOCUS)
MSG(WM_DESTROY)
MSG(WM_NCDESTROY)

/* we get these directly sent */
MSG(WM_NCHITTEST)
MSG(WM_SETCURSOR)
MSG(WM_MOUSEACTIVATE)
MSG(WM_CHILDACTIVATE)
MSG(WM_COMMAND)
MSG(WM_SYSCOMMAND)

/* posted events */
MSG(WM_MOUSEMOVE)
MSG(WM_NCMOUSEMOVE)
MSG(WM_PAINT)
MSG(WM_LBUTTONDOWN)
MSG(WM_LBUTTONUP)
MSG(WM_LBUTTONDBLCLK)
MSG(WM_NCLBUTTONDOWN)
MSG(WM_NCLBUTTONUP)
MSG(WM_NCLBUTTONDBLCLK)

MSG(WM_KEYDOWN)
MSG(WM_KEYUP)
MSG(WM_CHAR)

#ifdef WIN32
MSG(WM_CTLCOLORBTN)
MSG(WM_CTLCOLORDLG)
MSG(WM_CTLCOLORSTATIC)
#else
MSG(WM_CTLCOLOR)
#endif

/* moving and sizing */
MSG2(WM_ENTERSIZEMOVE,0x0231)
MSG2(WM_EXITSIZEMOVE,0x0232)
#ifdef WIN32
MSG(WM_SIZING)
#endif

/* menus/dialog boxes */
MSG(WM_CANCELMODE)
MSG(WM_ENABLE)
MSG(WM_SETFONT)
MSG(WM_INITDIALOG)
MSG(WM_GETDLGCODE)
MSG(WM_ENTERIDLE)

/* scroll bars */
MSG(WM_HSCROLL)
MSG(WM_VSCROLL)

/* getting these from Wine but not from Windows */
MSG2(WM_SETVISIBLE,0x0009) /* unheard of in BC++ 4.52 */
#ifdef WIN32
MSG(WM_CAPTURECHANGED)
#endif

ENDMSG};

struct MSGNAMES ButMsgs[]={
MSG(BM_SETSTATE)
MSG(BM_SETSTYLE)

ENDMSG};

char*MsgName(UINT msg,HWND hWnd)
{
 int i;
 static char buffer[64],wclass[64];
 GetClassName(hWnd,wclass,sizeof(wclass));

#define MSGSEARCH(msgs) { \
  for (i=0; msgs[i].name&&msgs[i].msg!=msg; i++); \
  if (msgs[i].name) return msgs[i].name; \
 }

 if (!stricmp(wclass,"Button")) MSGSEARCH(ButMsgs);
 MSGSEARCH(MsgNames);
 /* WM_USER */
 if (msg>=WM_USER) {
  sprintf(buffer,"WM_USER+%04x{%s}",msg-WM_USER,wclass);
  return buffer;
 }
 /* message not found */
 sprintf(buffer,"%04x{%s}",msg,wclass);
 return buffer;
}

char*WndName(HWND hWnd,int state)
{
 static char buffer[16];
 if (!hWnd) return "0000";
 if (hWnd==hMainWnd || (state==STATE_CREATE && !hMainWnd)) return "main";
 if (hWnd==hSubWnd || (state==STATE_CREATE && !hSubWnd)) return "chld";
 if (hWnd==hDialog || (state==STATE_DIALOG && !hDialog)) return "tdlg";
 if (hWnd==hGroup) return "tgrp";
 if (hWnd==hButton[0]) return "but1";
 if (hWnd==hButton[1]) return "but2";
 if (hWnd==hButton[2]) return "but3";
 if (hWnd==hButton[3]) return "but4";
 if (hWnd==hSubDlg || (state==STATE_CREATE && !hSubDlg)) return "sdlg";
 if (hDialog) {
  int id=GetDlgCtrlID(hWnd);
  if (id) {
   sprintf(buffer,"dlgitem(%d)",id);
   return buffer;
  }
 }
 sprintf(buffer,"%04x",hWnd);
 return buffer;
}

void Log(const char*fmt)
{
#ifdef LOGGING
 if (!Clicked) SendMessage(hListBox,LB_ADDSTRING,0,(LPARAM)fmt);
#endif
}

void Logf(const char*fmt,...)
{
 va_list par;
 static char buffer[256];

 va_start(par,fmt);
 vsprintf(buffer,fmt,par);
 va_end(par);
 Log(buffer);
}

void LogChildOrder(HWND hWnd)
{
 HWND hWndChild = GetWindow(hWnd,GW_CHILD);
 static char buffer[256];

 strcpy(buffer,"child list:");
 while (hWndChild) {
  strcat(strcat(buffer," "),WndName(hWndChild,State));
  hWndChild=GetWindow(hWndChild,GW_HWNDNEXT);
 }
 Log(buffer);
}

void LogMessage(int state,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam,char*name)
{
 static char buffer[256];
 DWORD tick=GetTickCount()-StartTime;
 char*msgname=MsgName(msg,hWnd);
 if (!name) name=WndName(hWnd,state);
 switch (msg) {
  case WM_SETFOCUS:
  case WM_KILLFOCUS:
  case WM_SETCURSOR:
   Logf("%04d[%s(%d):%s]%s(%s,%08x)",tick,StateName[state],Rec,
        name,msgname,WndName((HWND)wParam,State),lParam);
   break;
#ifdef WIN32
  case WM_ENTERIDLE:
  case WM_CTLCOLORBTN:
  case WM_CTLCOLORDLG:
   Logf("%04d[%s(%d):%s]%s(%08x,%s)",tick,StateName[state],Rec,
        name,msgname,wParam,WndName((HWND)lParam,State));
   break;
#else
  case WM_ENTERIDLE:
  case WM_CTLCOLOR:
   Logf("%04d[%s(%d):%s]%s(%08x,%04x:%s)",tick,StateName[state],Rec,
        name,msgname,wParam,HIWORD(lParam),WndName((HWND)LOWORD(lParam),State));
   break;
#endif
  case WM_WINDOWPOSCHANGING:
  case WM_WINDOWPOSCHANGED:
   {
    WINDOWPOS*pos=(WINDOWPOS*)lParam;
#ifdef WIN32
	 Logf("%04d[%s(%d):%s]%s(%08x,%p)",tick,StateName[state],Rec,
			name,msgname,wParam,pos);
#else
	 Logf("%04d[%s(%d):%s]%s(%04x,%p)",tick,StateName[state],Rec,
			name,msgname,wParam,pos);
#endif
	 strcpy(buffer,"FLAGS:");
	 if (pos->flags&SWP_DRAWFRAME) strcat(buffer," DRAWFRAME");
	 if (pos->flags&SWP_HIDEWINDOW) strcat(buffer," HIDEWINDOW");
	 if (pos->flags&SWP_NOACTIVATE) strcat(buffer," NOACTIVATE");
	 if (pos->flags&SWP_NOCOPYBITS) strcat(buffer," NOCOPYBITS");
	 if (pos->flags&SWP_NOMOVE) strcat(buffer," NOMOVE");
	 if (pos->flags&SWP_NOOWNERZORDER) strcat(buffer," NOOWNERZORDER");
	 if (pos->flags&SWP_NOSIZE) strcat(buffer," NOSIZE");
	 if (pos->flags&SWP_NOREDRAW) strcat(buffer," NOREDRAW");
	 if (pos->flags&SWP_NOZORDER) strcat(buffer," NOZORDER");
	 if (pos->flags&SWP_SHOWWINDOW) strcat(buffer," SHOWWINDOW");
	 Log(buffer);
	}
	break;
  case WM_SYSCOMMAND:
	{
	 char*cmd=NULL;
	 switch (wParam&0xFFF0) {
#define CASE(x) case SC_##x: cmd=#x; break;
	  CASE(CLOSE)
	  CASE(DEFAULT)
	  CASE(HOTKEY)
	  CASE(HSCROLL)
	  CASE(KEYMENU)
	  CASE(MAXIMIZE)
	  CASE(MINIMIZE)
	  CASE(MOUSEMENU)
	  CASE(MOVE)
	  CASE(NEXTWINDOW)
	  CASE(PREVWINDOW)
	  CASE(RESTORE)
	  CASE(SCREENSAVE)
	  CASE(SIZE)
	  CASE(TASKLIST)
	  CASE(VSCROLL)
#undef CASE
	 }
	 if (cmd) {
	  Logf("%04d[%s(%d):%s]%s(%s+%x,%08x)",tick,StateName[state],Rec,
			 name,msgname,cmd,wParam&0xF,lParam);
	 } else goto GENERIC_MSG;
	}
	break;
  case WM_HSCROLL:
  case WM_VSCROLL:
	{
	 char*cmd=NULL;
	 switch (LOWORD(wParam)) {
#define CASE(x) case SB_##x: cmd=#x; break;
#define CASE2(h,v) case SB_##h: if (msg==WM_HSCROLL) cmd=#h; else cmd=#v; break;
	  CASE(BOTTOM)
	  CASE(ENDSCROLL)
	  CASE2(LINELEFT,LINEUP)
	  CASE2(LINERIGHT,LINEDOWN)
	  CASE2(PAGELEFT,PAGEUP)
	  CASE2(PAGERIGHT,PAGEDOWN)
	  CASE(THUMBPOSITION)
	  CASE(THUMBTRACK)
     CASE(TOP)
#undef CASE
	 }
	 if (cmd) {
#ifdef WIN32
	  Logf("%04d[%s(%d):%s]%s(%s,%04x,%s)",tick,StateName[state],Rec,
			 name,msgname,cmd,HIWORD(wParam),WndName((HWND)lParam,State));
#else
	  Logf("%04d[%s(%d):%s]%s(%04x,%04x,%s)",tick,StateName[state],Rec,
			 name,msgname,cmd,LOWORD(lParam),WndName((HWND)HIWORD(lParam),State));
#endif
	 } else goto GENERIC_MSG;
	}
	break;
  default:
GENERIC_MSG:
#ifdef WIN32
	Logf("%04d[%s(%d):%s]%s(%08x,%08x)",tick,StateName[state],Rec,
		  name,msgname,wParam,lParam);
#else
	Logf("%04d[%s(%d):%s]%s(%04x,%08x)",tick,StateName[state],Rec,
		  name,msgname,wParam,lParam);
#endif
 }
}

/***************************/
/*** GRAPHICS FACILITIES ***/
/***************************/

void Paint(HWND hWnd)
{
 HDC dc;
 PAINTSTRUCT ps;
 dc=BeginPaint(hWnd,&ps);
 EndPaint(hWnd,&ps);
}

void FillPattern(HWND hWnd,HDC pdc)
{
 HDC dc=pdc?pdc:GetDC(hWnd);
 HBRUSH oldbrush;
 RECT rect;
 if (!dc) {
  Logf("failed to acquire DC for window %s",WndName(hWnd,State));
  return;
 } else {
  Logf("acquired DC for %s window %s, painting",
       IsWindowVisible(hWnd)?"visible":"invisible",WndName(hWnd,State));
 }
 GetClientRect(hWnd,&rect);
 oldbrush=SelectObject(dc,GetStockObject(LTGRAY_BRUSH));
 PatBlt(dc,0,0,rect.right,rect.bottom,PATCOPY);
 SelectObject(dc,oldbrush);
 if (!pdc) ReleaseDC(hWnd,dc);
}

void PaintPattern(HWND hWnd)
{
 HDC dc;
 PAINTSTRUCT ps;
 dc=BeginPaint(hWnd,&ps);
 FillPattern(hWnd,dc);
 EndPaint(hWnd,&ps);
}

/*************************/
/*** WINDOW PROCEDURES ***/
/*************************/

/* MAIN WINDOW */
LRESULT FAR CALLBACK _export MainWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
 LRESULT lResult=0;
 RECT rect;
 int OldState=State;

 State=STATE_RECURS; Rec++;
 if (!Clicked) LogMessage(OldState,hWnd,msg,wParam,lParam,NULL);
 switch (msg) {
  case WM_NCHITTEST:
   lResult=DefWindowProc(hWnd,msg,wParam,lParam);
   break;
  case WM_LBUTTONDOWN:
  case WM_CHAR:
   if (!Clicked) {
    SetParent(hListBox,hWnd);
    GetClientRect(hWnd,&rect);
    MoveWindow(hListBox,0,0,rect.right,rect.bottom,TRUE);
    ShowWindow(hListBox,SW_SHOW);
    SetFocus(hListBox);
    Clicked=TRUE;
   }
   break;
  case WM_SIZE:
   GetClientRect(hWnd,&rect);
   if (Clicked) {
    MoveWindow(hListBox,0,0,rect.right,rect.bottom,TRUE);
   }
   MoveWindow(hSubWnd,0,rect.bottom/2,rect.right,rect.bottom-(rect.bottom/2),TRUE);
   break;
  case WM_PAINT:
   Paint(hWnd);
   break;
  case WM_DESTROY:
   PostQuitMessage(0);
   break;
  default:
   lResult=DefWindowProc(hWnd,msg,wParam,lParam);
 }
 State=OldState; Rec--;
 return lResult;
}

/* CHILD WINDOW */
LRESULT FAR CALLBACK _export SubWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
 LRESULT lResult=0;
 RECT rect;
 int OldState=State;

 State=STATE_RECURS; Rec++;
 if (!Clicked) LogMessage(OldState,hWnd,msg,wParam,lParam,NULL);
 switch (msg) {
  case WM_PAINT:
   Paint(hWnd);
   break;
  default:
   lResult=DefWindowProc(hWnd,msg,wParam,lParam);
 }
 State=OldState; Rec--;
 return lResult;
}

BOOL FAR CALLBACK _export SubDialogProc(HWND hWndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

/* SUBCLASSED CONTROLS */
LRESULT FAR CALLBACK _export SubClassWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
 LRESULT lResult=0;
 RECT rect;
 int OldState=State;
 int But=-1;

 if (hWnd==hButton[0]) But=0; else
 if (hWnd==hButton[1]) But=1; else
 if (hWnd==hButton[2]) But=2; else
 if (hWnd==hButton[3]) But=3;

 State=STATE_RECURS; Rec++;
 if (!Clicked) {
  LogMessage(OldState,hWnd,msg,wParam,lParam,NULL);
  if (But!=-1) {
   lResult=CallWindowProc((FARPROC)wndButton[But],hWnd,msg,wParam,lParam);
   if (msg==WM_LBUTTONUP) {
    LogChildOrder(GetParent(hWnd));
   }
  }
  else if (hWnd==hDialog) {
   lResult=CallWindowProc((FARPROC)wndDialog,hWnd,msg,wParam,lParam);
  }
  else if (hWnd==hSubDlg) {
   lResult=CallWindowProc((FARPROC)wndSubDlg,hWnd,msg,wParam,lParam);
  }
  else if (hWnd==hGroup) {
   lResult=CallWindowProc((FARPROC)wndGroup,hWnd,msg,wParam,lParam);
   if (msg==WM_SETFOCUS) {
    /* create subdialog */
    if (hSubDlg) {
#if 0
     SetRect(&rect,0,0,1,1);
     InvalidateRect(hWnd,&rect,FALSE);
#endif
    } else {
#ifdef TEST_SUBDIALOG
     State=STATE_CREATE;
     hSubDlg=CreateDialog(hInst,MAKEINTRESOURCE(2),hWnd,(FARPROC)SubDialogProc);
     State=STATE_RECURS;
#else
#ifdef RESIZE_DIALOG
     GetWindowRect(GetParent(hWnd),&rect);
     rect.right++;
     SetWindowPos(GetParent(hWnd),0,0,0,
                  rect.right-rect.left,rect.bottom-rect.top,
                  SWP_NOMOVE|SWP_NOZORDER);
#endif
#endif
    }
   }
  }
 }
 State=OldState; Rec--;
 return lResult;
}

/* MAIN DIALOG PROCEDURE */
BOOL FAR CALLBACK _export TestDialogProc(HWND hWndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
 BOOL bResult=0;
 RECT rect;
 int OldState=State;
 int But=-1;

 State=STATE_RECURS; Rec++;
 if (!Clicked) LogMessage(OldState,hWndDlg,msg,wParam,lParam,"dlgp");
 switch (msg) {
  case WM_INITDIALOG:
   hDialog = hWndDlg;
   /* subclass dialog window proc */
   wndDialog = (WNDPROC)SetWindowLong(hDialog,GWL_WNDPROC,(LONG)SubClassWindowProc);
   Logf("dialog visible=%s",IsWindowVisible(hWndDlg)?"TRUE":"FALSE");
   /* subclass OK button */
   hButton[3] = GetDlgItem(hWndDlg,IDOK);
   wndButton[3] = (WNDPROC)SetWindowLong(hButton[3],GWL_WNDPROC,(LONG)SubClassWindowProc);
   /* subclass group box */
   hGroup = GetDlgItem(hWndDlg,IDC_GROUPBOX1);
   wndGroup = (WNDPROC)SetWindowLong(hGroup,GWL_WNDPROC,(LONG)SubClassWindowProc);

#ifdef RESIZE_DIALOG
   GetWindowRect(hWndDlg,&rect);
   rect.right--;
   SetWindowPos(hWndDlg,0,0,0,
                rect.right-rect.left,rect.bottom-rect.top,
                SWP_NOMOVE|SWP_NOZORDER);
//   ShowWindow(GetDlgItem(hWndDlg,IDCANCEL),SW_HIDE);
#endif

   bResult=TRUE; /* we don't do SetFocus */
   break;
  case WM_PAINT:
   PaintPattern(hWndDlg);
   bResult=TRUE;
   break;
  case WM_COMMAND:
   EndDialog(hWndDlg,LOWORD(wParam));
   bResult=TRUE;
   break;
  case WM_CLOSE:
   EndDialog(hWndDlg,IDCANCEL);
   bResult=TRUE;
   break;
  case WM_NCDESTROY:
   hDialog = 0;
   break;
 }
 State=OldState; Rec--;
 return bResult;
}

/* SUBDIALOG PROCEDURE */
BOOL FAR CALLBACK _export SubDialogProc(HWND hWndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
 BOOL bResult=0;
 RECT rect;
 int OldState=State;
 int But=-1;

 State=STATE_RECURS; Rec++;
 if (!Clicked) LogMessage(OldState,hWndDlg,msg,wParam,lParam,NULL);
 switch (msg) {
  case WM_INITDIALOG:
   hSubDlg = hWndDlg;
   /* subclass dialog window proc */
   wndSubDlg = (WNDPROC)SetWindowLong(hDialog,GWL_WNDPROC,(LONG)SubClassWindowProc);

   bResult=TRUE; /* we don't do SetFocus */
   break;
  case WM_NCDESTROY:
   hSubDlg = 0;
   break;
 }
 State=OldState; Rec--;
 return bResult;
}

/********************/
/*** MAIN PROGRAM ***/
/********************/

BOOL AppInit(void)
{
 WNDCLASS wclass;

 wclass.style = CS_HREDRAW|CS_VREDRAW;
 wclass.lpfnWndProc = MainWindowProc;
 wclass.cbClsExtra = 0;
 wclass.cbWndExtra = 0;
 wclass.hInstance = hInst;
 wclass.hIcon = LoadIcon(hInst,MAKEINTRESOURCE(1));
 wclass.hCursor = LoadCursor(0,IDC_ARROW);
 wclass.hbrBackground = GetStockObject(WHITE_BRUSH);
 wclass.lpszMenuName = NULL;
 wclass.lpszClassName = wclassname;
 if (!RegisterClass(&wclass)) return FALSE;
 wclass.lpfnWndProc = SubWindowProc;
 wclass.lpszClassName = wcclassname;
 if (!RegisterClass(&wclass)) return FALSE;
 return TRUE;
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int nCmdShow)
{
 MSG msg;
 RECT rect;

 hInst = hInstance;
 if (!hPrevInstance)
  if (!AppInit())
   return 0;

 StartTime=GetTickCount();
 hListBox = CreateWindow("LISTBOX","Messages",WS_BORDER|WS_VSCROLL|WS_CHILD|
                                              LBS_HASSTRINGS|LBS_NOTIFY|LBS_WANTKEYBOARDINPUT,
                         0,0,0,0,GetDesktopWindow(),0,hInst,0);
 if (!hListBox) {
  MessageBox(0,"Could not create list box","Error",MB_OK);
 }

 State=STATE_CREATE;
 hMainWnd = CreateWindowEx(MAIN_EXSTYLE,wclassname,winname,MAIN_STYLE,
                           CW_USEDEFAULT,0,400,300,0,0,hInst,0);
 if (!hMainWnd) return 0;
 State=STATE_SHOW;
 ShowWindow(hMainWnd,nCmdShow);
#ifdef TEST_DESTROY_MAIN
 State=STATE_DESTROY;
 DestroyWindow(hMainWnd);
 State=STATE_DIRECT;
 while (GetMessage(&msg,0,0,0)) {
  TranslateMessage(&msg);
  State=STATE_DISPATCH;
  DispatchMessage(&msg);
  State=STATE_DIRECT;
 }
 State=STATE_CREATE;
 hMainWnd = CreateWindowEx(MAIN_EXSTYLE,wclassname,winname,MAIN_STYLE,
                           CW_USEDEFAULT,0,400,300,0,0,hInst,0);
 if (!hMainWnd) return 0;
 State=STATE_SHOW;
 ShowWindow(hMainWnd,nCmdShow);
#endif
/* update, so no WM_PAINTs are pending */
 State=STATE_UPDATE;
// UpdateWindow(hMainWnd);
 Ready=TRUE;
/* fill client area with a pattern */
 FillPattern(hMainWnd,0);
/* create subwindow */
 State=STATE_CREATE;
 GetClientRect(hMainWnd,&rect);
 hSubWnd = CreateWindow(wcclassname,winname,WS_CHILD|WS_BORDER|WS_CLIPSIBLINGS,
                        0,rect.bottom/2,rect.right,rect.bottom-(rect.bottom/2),hMainWnd,0,hInst,0);
 if (!hSubWnd) return 0;
/* create buttons */
 hButton[0] = CreateWindow("BUTTON","1",WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
                           8,8,48,20,hMainWnd,0,hInst,0);
 hButton[1] = CreateWindow("BUTTON","2",WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
                           32,12,48,20,hMainWnd,0,hInst,0);
 hButton[2] = CreateWindow("BUTTON","3",WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
                           56,16,48,20,hMainWnd,0,hInst,0);
/* subclass them */
 wndButton[0] = (WNDPROC)SetWindowLong(hButton[0],GWL_WNDPROC,(LONG)SubClassWindowProc);
 wndButton[1] = (WNDPROC)SetWindowLong(hButton[1],GWL_WNDPROC,(LONG)SubClassWindowProc);
 wndButton[2] = (WNDPROC)SetWindowLong(hButton[2],GWL_WNDPROC,(LONG)SubClassWindowProc);
/* show them */
 State=STATE_UPDATE;
 UpdateWindow(hButton[0]);
 LogChildOrder(hMainWnd);
 Logf("but1 visible=%d",IsWindowVisible(hButton[0]));

/* now reparent the button to our (invisible) subwindow */
 State=STATE_TEST;
 /* in different order, seeing who gets topmost */
 SetParent(hButton[0],hSubWnd);
 SetParent(hButton[2],hSubWnd);
 SetParent(hButton[1],hSubWnd);
 LogChildOrder(hSubWnd);
/* the button should now be invisible */
 Logf("but1 visible=%d",IsWindowVisible(hButton[0]));
/* see if we can draw on them */
 FillPattern(hButton[0],0);

#ifdef SHOW_SUB
 State=STATE_SHOW;
 ShowWindow(hSubWnd,SW_SHOWNORMAL);
 State=STATE_UPDATE;
 UpdateWindow(hSubWnd);
 FillPattern(hSubWnd,0);
// InvalidateRect(hMainWnd,NULL,TRUE);
 Logf("but1 visible=%d",IsWindowVisible(hButton[0]));
#endif

#ifdef TEST_DIALOG
 State=STATE_DIALOG;
 DialogBox(hInst,MAKEINTRESOURCE(1),hMainWnd,(FARPROC)TestDialogProc);
#endif
#ifdef TEST_COMMCTL
 {
  DWORD arr[16];
  CHOOSECOLOR cc={sizeof(cc),0,hInst,0,arr,0};
  ChooseColor(&cc);
 }
#endif

 State=STATE_DIRECT;
 while (GetMessage(&msg,0,0,0)) {
  TranslateMessage(&msg);
  State=STATE_DISPATCH;
  DispatchMessage(&msg);
  State=STATE_DIRECT;
 }
 return 0;
}


