#include <windows.h>
#include <stdio.h>

typedef struct _THRDCREATEWIN
{
  HANDLE hThread;
  DWORD ThreadId;
  LPSTR Caption;
  HWND *Parent;
  HWND Window;
  DWORD Style;
  POINT Position;
  SIZE Size;
} THRDCREATEWIN, *PTHRDCREATEWIN;

static HINSTANCE hAppInstance;
static HANDLE WinCreatedEvent;

LRESULT WINAPI MultiWndProc(HWND, UINT, WPARAM, LPARAM);

DWORD WINAPI
WindowThreadProc(LPVOID lpParameter)
{
  MSG msg;
  char caption[64];
  PTHRDCREATEWIN cw = (PTHRDCREATEWIN)lpParameter;
  
  sprintf(caption, cw->Caption, GetCurrentThreadId());
  
  cw->Window = CreateWindow("MultiClass",
                            caption,
                            cw->Style | WS_VISIBLE,
                            cw->Position.x,
                            cw->Position.y,
                            cw->Size.cx,
                            cw->Size.cy,
                            (cw->Parent ? *(cw->Parent) : 0),
                            NULL,
                            hAppInstance,
                            NULL);
  
  SetEvent(WinCreatedEvent);
  
  if(!cw->Window)
  {
    fprintf(stderr, "CreateWindow failed (last error 0x%lX)\n",
            GetLastError());
    return 1;
  }
  
  while(GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  
  return 0;
}

int WINAPI 
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  WNDCLASS wc;
  int i;
  static THRDCREATEWIN wnds[3];
  HANDLE Threads[3];
  
  hAppInstance = hInstance;
  
  WinCreatedEvent = CreateEvent(NULL,
                                FALSE,
                                FALSE,
                                NULL);
  
  if(!WinCreatedEvent)
  {
    fprintf(stderr, "Failed to create event (last error 0x%lX)\n",
            GetLastError());
    return 1;
  }

  wc.lpszClassName = "MultiClass";
  wc.lpfnWndProc = MultiWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, (LPCTSTR) IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, (LPCTSTR) IDC_ARROW);
  wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClass(&wc) == 0)
    {
      fprintf(stderr, "RegisterClass failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }

  wnds[0].Caption = "TopLevel1 (ThreadID: %d)";
  wnds[0].Parent = NULL;
  wnds[0].Style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
  wnds[0].Position.x = wnds[0].Position.y = 0;
  wnds[0].Size.cx = 320;
  wnds[0].Size.cy = 240;
  
  wnds[1].Caption = "Child1 of TopLevel1 (ThreadID: %d)";
  wnds[1].Parent = &wnds[0].Window;
  wnds[1].Style = WS_CHILD | WS_BORDER | WS_CAPTION | WS_VISIBLE | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
  wnds[1].Position.x = 20;
  wnds[1].Position.y = 120;
  wnds[1].Size.cx = wnds[1].Size.cy = 240;
  
  wnds[2].Caption = "TopLevel2 (ThreadID: %d)";
  wnds[2].Parent = NULL;
  wnds[2].Style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
  wnds[2].Position.x = 400;
  wnds[2].Position.y = 0;
  wnds[2].Size.cx = 160;
  wnds[2].Size.cy = 490;
  
  for(i = 0; i < (sizeof(wnds) / sizeof(THRDCREATEWIN)); i++)
  {
    wnds[i].hThread = CreateThread(NULL,
                                   0,
                                   WindowThreadProc,
                                   &wnds[i],
                                   0,
                                   &wnds[i].ThreadId);
    Threads[i] = wnds[i].hThread;
    if(!wnds[i].hThread)
    {
      fprintf(stderr, "CreateThread #%i failed (last error 0x%lX)\n",
              i, GetLastError());
      return 1;
    }
    WaitForSingleObject(WinCreatedEvent, INFINITE);
  }
  
  WaitForMultipleObjects(sizeof(Threads) / sizeof(HANDLE), &Threads[0], TRUE, INFINITE);
  
  UnregisterClass("MultiClass", hInstance);
  
  return 0;
}

LRESULT CALLBACK MultiWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hDC;
  RECT Client;
  HBRUSH Brush;
  static COLORREF Colors[] =
    {
      RGB(0x00, 0x00, 0x00),
      RGB(0x80, 0x00, 0x00),
      RGB(0x00, 0x80, 0x00),
      RGB(0x00, 0x00, 0x80),
      RGB(0x80, 0x80, 0x00),
      RGB(0x80, 0x00, 0x80),
      RGB(0x00, 0x80, 0x80),
      RGB(0x80, 0x80, 0x80),
      RGB(0xff, 0x00, 0x00),
      RGB(0x00, 0xff, 0x00),
      RGB(0x00, 0x00, 0xff),
      RGB(0xff, 0xff, 0x00),
      RGB(0xff, 0x00, 0xff),
      RGB(0x00, 0xff, 0xff),
      RGB(0xff, 0xff, 0xff)
    };
  static unsigned CurrentColor = 0;

  switch(msg)
    {
      case WM_PAINT:
	hDC = BeginPaint(hWnd, &ps);
	GetClientRect(hWnd, &Client);
	Brush = CreateSolidBrush(Colors[CurrentColor]);
	FillRect(hDC, &Client, Brush);
	DeleteObject(Brush);
	CurrentColor++;
	if (sizeof(Colors) / sizeof(Colors[0]) <= CurrentColor)
	  {
	    CurrentColor = 0;
	  }
	EndPaint(hWnd, &ps);
	break;

      case WM_DESTROY:
	PostQuitMessage(0);
	break;

      default:
	return DefWindowProc(hWnd, msg, wParam, lParam);
    }

  return 0;
}
