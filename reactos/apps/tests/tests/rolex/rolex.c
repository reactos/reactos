/*********************************************************************
 *                                                                   *
 *  rolex.c: Windows clock application for WINE (by Jim Peterson)    *
 *                                                                   *
 *  This is a translation of a Turbo Pascal OWL application I made   *
 *  once, so it's a little flaky (tons of globals, functions that    *
 *  could have been in-lined, etc.).  The source code should easily  *
 *  compile with a standard Win32 C compiler.                        *
 *                                                                   *
 *  To try it out, type 'make rolex'.                                *
 *                                                                   *
 *********************************************************************/

#include <math.h>
#include <string.h>
#include "windows.h"

char AppName[] = "Rolex";
char WindowName[] = "Rolex";
int WindowWidth = 100;
int WindowHeight = 121;
COLORREF FaceColor = RGB(192,192,192);
COLORREF HandColor = RGB(0,0,0);
COLORREF EtchColor = RGB(0,0,0);

float Pi=3.1415926;

typedef struct
{
  int StartX,StartY,EndX,EndY;
} HandData;

int MaxX,MaxY;
HandData OldSecond,OldHour,OldMinute;

HWND HWindow;

void DrawFace(HDC dc)
{
  int MidX, MidY, t;

  MidX=MaxX/2;
  MidY=MaxY/2;
  SelectObject(dc,CreateSolidBrush(FaceColor));
  SelectObject(dc,CreatePen(PS_SOLID,1,EtchColor));
  Ellipse(dc,0,0,MaxX,MaxY);

  for(t=0; t<12; t++)
  {
    MoveToEx(dc,MidX+sin(t*Pi/6)*0.9*MidX,MidY-cos(t*Pi/6)*0.9*MidY,NULL);
    LineTo(dc,MidX+sin(t*Pi/6)*0.8*MidX,MidY-cos(t*Pi/6)*0.8*MidY);
  }
  if(MaxX>64 && MaxY>64)
    for(t=0; t<60; t++)
      SetPixel(dc,MidX+sin(t*Pi/30)*0.9*MidX,MidY-cos(t*Pi/30)*0.9*MidY
	       ,EtchColor);
  DeleteObject(SelectObject(dc,GetStockObject(NULL_BRUSH)));
  DeleteObject(SelectObject(dc,GetStockObject(NULL_PEN)));
  memset(&OldSecond,0,sizeof(OldSecond));
  memset(&OldMinute,0,sizeof(OldMinute));
  memset(&OldHour,0,sizeof(OldHour));
}

void DrawHourHand(HDC dc)
{
  MoveToEx(dc, OldHour.StartX, OldHour.StartY, NULL);
  LineTo(dc, OldHour.EndX, OldHour.EndY);
}

void DrawMinuteHand(HDC dc)
{
  MoveToEx(dc, OldMinute.StartX, OldMinute.StartY, NULL);
  LineTo(dc, OldMinute.EndX, OldMinute.EndY);
}

void DrawSecondHand(HDC dc)
{
  MoveToEx(dc, OldSecond.StartX, OldSecond.StartY, NULL);
  LineTo(dc, OldSecond.EndX, OldSecond.EndY);
}

BOOL UpdateHourHand(HDC dc,int MidX,int MidY,int XExt,int YExt,WORD Pos)
{
  int Sx, Sy, Ex, Ey;
  BOOL rv;

  rv = FALSE;
  Sx = MidX; Sy = MidY;
  Ex = MidX+sin(Pos*Pi/6000)*XExt;
  Ey = MidY-cos(Pos*Pi/6000)*YExt;
  rv = ( Sx!=OldHour.StartX || Ex!=OldHour.EndX || 
	 Sy!=OldHour.StartY || Ey!=OldHour.EndY );
  if(rv)DrawHourHand(dc);
  OldHour.StartX = Sx; OldHour.EndX = Ex;
  OldHour.StartY = Sy; OldHour.EndY = Ey;
  return rv;
}

BOOL UpdateMinuteHand(HDC dc,int MidX,int MidY,int XExt,int YExt,WORD Pos)
{
  int Sx, Sy, Ex, Ey;
  BOOL rv;

  rv = FALSE;
  Sx = MidX; Sy = MidY;
  Ex = MidX+sin(Pos*Pi/30000)*XExt;
  Ey = MidY-cos(Pos*Pi/30000)*YExt;
  rv = ( Sx!=OldMinute.StartX || Ex!=OldMinute.EndX ||
	 Sy!=OldMinute.StartY || Ey!=OldMinute.EndY );
  if(rv)DrawMinuteHand(dc);
  OldMinute.StartX = Sx; OldMinute.EndX = Ex;
  OldMinute.StartY = Sy; OldMinute.EndY = Ey;
  return rv;
}

BOOL UpdateSecondHand(HDC dc,int MidX,int MidY,int XExt,int YExt,WORD Pos)
{
  int Sx, Sy, Ex, Ey;
  BOOL rv;

  rv = FALSE;
  Sx = MidX; Sy = MidY;
  Ex = MidX+sin(Pos*Pi/3000)*XExt;
  Ey = MidY-cos(Pos*Pi/3000)*YExt;
  rv = ( Sx!=OldSecond.StartX || Ex!=OldSecond.EndX ||
	 Sy!=OldSecond.StartY || Ey!=OldSecond.EndY );
  if(rv)DrawSecondHand(dc);
  OldSecond.StartX = Sx; OldSecond.EndX = Ex;
  OldSecond.StartY = Sy; OldSecond.EndY = Ey;
  return rv;
}

void Idle(HDC idc)
{
  SYSTEMTIME st;
  WORD H, M, S, F;
  int MidX, MidY;
  HDC dc;
  BOOL Redraw;

  if(idc)
    dc=idc;
  else
    dc=GetDC(HWindow);
  if(!dc)return;

  GetLocalTime(&st);
  H = st.wHour;
  M = st.wMinute;
  S = st.wSecond;
  F = st.wMilliseconds / 10;
  F = F + S*100;
  M = M*1000+F/6;
  H = H*1000+M/60;
  MidX = MaxX/2;
  MidY = MaxY/2;
  SelectObject(dc,CreatePen(PS_SOLID,1,FaceColor));
  Redraw = FALSE;
  if(UpdateHourHand(dc,MidX,MidY,MidX*0.5,MidY*0.5,H)) Redraw = TRUE;
  if(UpdateMinuteHand(dc,MidX,MidY,MidX*0.65,MidY*0.65,M)) Redraw = TRUE;
  if(UpdateSecondHand(dc,MidX,MidY,MidX*0.79,MidY*0.79,F)) Redraw = TRUE;
  DeleteObject(SelectObject(dc,CreatePen(PS_SOLID,1,HandColor)));
  if(Redraw)
  {
    DrawSecondHand(dc);
    DrawMinuteHand(dc);
    DrawHourHand(dc);
  }
  DeleteObject(SelectObject(dc,GetStockObject(NULL_PEN)));
  if(!idc) ReleaseDC(HWindow,dc);
}

LRESULT CALLBACK ProcessAppMsg(HWND wnd,UINT msg,WPARAM w,LPARAM l)
{
  PAINTSTRUCT PaintInfo;
  HDC dc;

  switch(msg)
  {
  case WM_PAINT:
    if(GetUpdateRect(wnd,NULL,FALSE))
    {
      dc=BeginPaint(wnd,&PaintInfo);
      DrawFace(dc);
      Idle(dc);
      EndPaint(wnd,&PaintInfo);
    }
    break;

  case WM_SIZE:
    MaxX = LOWORD(l);
    MaxY = HIWORD(l);
    break;

  case WM_DESTROY:
    PostQuitMessage (0);
    break;

  default:
    return DefWindowProc (wnd, msg, w, l);
  }
  return 0l;
}

WPARAM MessageLoop()
{
  MSG msg;

  while(1)
  {
    Sleep(1); /* sleep 1 millisecond */
    if(PeekMessage(&msg,0,0,0,PM_REMOVE))
    {
      if(msg.message == WM_QUIT) return msg.wParam;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else
      Idle(NULL);
  }
}

int PASCAL WinMain (HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show)
{
  WNDCLASS class;
  if(!prev)
  {
    class.style = CS_HREDRAW | CS_VREDRAW;
    class.lpfnWndProc = ProcessAppMsg;
    class.cbClsExtra = 0;
    class.cbWndExtra = 0;
    class.hInstance  = inst;
    class.hIcon      = 0; /* Draw my own icon */
    class.hCursor    = LoadCursor (0, IDC_ARROW);
    class.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    class.lpszMenuName = 0;
    class.lpszClassName = AppName;
  }
  if (!RegisterClass (&class)) return -1;

  HWindow=CreateWindowEx(WS_EX_TOPMOST,AppName,WindowName,WS_OVERLAPPEDWINDOW,
			 CW_USEDEFAULT,CW_USEDEFAULT,WindowWidth,WindowHeight,
			 0,0,inst,0);
  memset(&OldSecond,0,sizeof(OldSecond));
  memset(&OldMinute,0,sizeof(OldMinute));
  memset(&OldHour,0,sizeof(OldHour));
  MaxX = WindowWidth;
  MaxY = WindowHeight;

  ShowWindow (HWindow, show);
  UpdateWindow (HWindow);

  return MessageLoop();
}
