#include <windows.h>
#include <stdio.h>
#include <tchar.h>

HFONT tf;
LRESULT WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOLEAN bolWM_CHAR;
BOOLEAN bolWM_KEYDOWN;

int WINAPI 
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  WNDCLASS wc;
  MSG msg;
  HWND hWnd;
  bolWM_CHAR = 0;
  bolWM_KEYDOWN = 0;

  wc.lpszClassName = "HelloClass";
  wc.lpfnWndProc = MainWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, (LPCTSTR)IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, (LPCTSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClass(&wc) == 0)
    {
      fprintf(stderr, "RegisterClass failed (last error 0x%X)\n",
	      GetLastError());
      return(1);
    }

  hWnd = CreateWindow("HelloClass",
	          "Hello World",
	          WS_OVERLAPPEDWINDOW|WS_HSCROLL|WS_VSCROLL,
	          0, //Position; you can use CW_USEDEFAULT, too
	          0,
	          600, //height
	          400,
	          NULL,
	          NULL,
	          hInstance,
	          NULL);
  if (hWnd == NULL)
    {
      fprintf(stderr, "CreateWindow failed (last error 0x%X)\n",
	      GetLastError());
      return(1);
    }

	tf = CreateFontA(14, 0, 0, TA_BASELINE, FW_NORMAL, FALSE, FALSE, FALSE,
	    ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	    DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "Timmons");

  ShowWindow(hWnd, nCmdShow);

  while(GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  DeleteObject(tf);

  return msg.wParam;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	RECT clr, wir;
        char spr[100], sir[100];
	static HBRUSH hbrWhite=NULL, hbrGray=NULL, hbrBlack=NULL, hbrRed=NULL, hbrBlue=NULL, hbrYellow=NULL;

	switch(msg)
	{
	     
	  case WM_CHAR:
	  { 
	    
	   hDC = GetDC(hWnd);
	   TCHAR text[2];
 	   text[0] = (TCHAR)wParam;
	   text[1] = _T('\0');
	   
	   
	   //Write in window
	   if( bolWM_KEYDOWN )
	   {
       	        TextOut(hDC, 400, 10, "WM CHAR:", strlen("WM CHAR:"));
       	        bolWM_KEYDOWN = 0;
	   }
	   else
	   {
	        TextOut(hDC, 400, 10, "WM_CHAR:", strlen("WM_CHAR:"));
	        bolWM_KEYDOWN = 1;
	   }
	   TextOut(hDC, 530, 10, text, strlen(text)); 
	   
	   /*
	   //Make a line depending on the typed key
	   Rect.left = 10;
	   Rect.top = 75;
	   Rect.right = 610;
	   Rect.bottom = 85;
	   FillRect(hDC, &Rect, hbrWhite);
	      
	   Rect.left=308;
	   Rect.right=312;
	   FillRect(hDC, &Rect, hbrRed);
	       
	   Rect.left = 310;
	   Rect.top = 75;
	   Rect.right = 310 +text[0]*2;
	   Rect.bottom = 85;
	   HBRUSH hbrCustom = CreateSolidBrush ( RGB(text[0], text[0], text[0]));
	   FillRect(hDC, &Rect, hbrCustom);
	   DeleteObject ( hbrCustom );
	   */
	   ReleaseDC(hWnd, hDC);
	   return 0;
	 }

	  case WM_KEYDOWN:
	  { 
	    
	   hDC = GetDC(hWnd);
	   RECT Rect;
	   TCHAR text[2];
 	   text[0] = (TCHAR)wParam;
	   text[1] = _T('\0');
	   
	   
	   //Write in window
	   Rect.left = 400;
	   Rect.top = 50;
	   Rect.right = 550;
	   Rect.bottom = 70;
	   FillRect(hDC, &Rect, hbrWhite);
	   if( bolWM_CHAR )
	   {
	        TextOut(hDC, 400, 30, "WM KEYDOWN:", strlen("WM KEYDOWN:"));
	        bolWM_CHAR = 0;
	   }
	   else
	   {
	        TextOut(hDC, 400, 30, "WM_KEYDOWN:", strlen("WM_KEYDOWN:"));
	        bolWM_CHAR = 1;
	   }
	   TextOut(hDC, 530, 30, text, strlen(text));
	   ReleaseDC(hWnd, hDC);
	   return 0;
	 }

	  case WM_KEYUP:
	  { 
	    
	   hDC = GetDC(hWnd);
	   RECT Rect;
	   TCHAR text[2];
 	   text[0] = (TCHAR)wParam;
	   text[1] = _T('\0');
	   

	   //Write in window
	   Rect.left = 400;
	   Rect.top = 10;
	   Rect.right = 550;
	   Rect.bottom = 70;
	   FillRect(hDC, &Rect, hbrWhite);
	   TextOut(hDC, 400, 50, "WM_KEYUP:", strlen("WM_KEYUP:"));
	   TextOut(hDC, 530, 50, text, strlen(text)); 
	   ReleaseDC(hWnd, hDC);
	   return 0;
	 }


        case WM_LBUTTONDOWN:
	 {
	   ULONG x, y;
	   RECT Rect;
	   hDC = GetDC(hWnd);
	   x = LOWORD(lParam);
	   y = HIWORD(lParam);
	   
	   Rect.left = x - 5;
	   Rect.top = y - 5;
	   Rect.right = x + 5;
	   Rect.bottom = y + 5;
	   FillRect(hDC, &Rect, hbrRed);
	   
	   Rect.left = x - 3;
	   Rect.top = y - 3;
	   Rect.right = x + 3;
	   Rect.bottom = y + 3;
	   FillRect(hDC, &Rect, hbrBlack);

           ReleaseDC(hWnd, hDC);
	   break;
	 }
        case WM_LBUTTONUP:
	 {
	   ULONG x, y;
	   RECT Rect;
	   hDC = GetDC(hWnd);
	   x = LOWORD(lParam);
	   y = HIWORD(lParam);
	   
	   Rect.left = x - 5;
	   Rect.top = y - 5;
	   Rect.right = x + 5;
	   Rect.bottom = y + 5;
	   FillRect(hDC, &Rect, hbrRed);
	   
	   Rect.left = x - 3;
	   Rect.top = y - 3;
	   Rect.right = x + 3;
	   Rect.bottom = y + 3;
	   FillRect(hDC, &Rect, hbrGray);
	   
	   ReleaseDC(hWnd, hDC);
	   break;
	 }	  
        case WM_MBUTTONDOWN:
	 {
	   ULONG x, y;
	   RECT Rect;
	   hDC = GetDC(hWnd);
	   x = LOWORD(lParam);
	   y = HIWORD(lParam);
	   
	   Rect.left = x - 5;
	   Rect.top = y - 5;
	   Rect.right = x + 5;
	   Rect.bottom = y + 5;
	   FillRect(hDC, &Rect, hbrBlue);
	   
	   Rect.left = x - 3;
	   Rect.top = y - 3;
	   Rect.right = x + 3;
	   Rect.bottom = y + 3;
	   FillRect(hDC, &Rect, hbrBlack);
	   
	   ReleaseDC(hWnd, hDC);
	   break;
	 }
        case WM_MBUTTONUP:
	 {
	   ULONG x, y;
	   RECT Rect;
	   hDC = GetDC(hWnd);
	   x = LOWORD(lParam);
	   y = HIWORD(lParam);
	   
	   Rect.left = x - 5;
	   Rect.top = y - 5;
	   Rect.right = x + 5;
	   Rect.bottom = y + 5;
	   FillRect(hDC, &Rect, hbrBlue);
	    	    
	   Rect.left = x - 3;
	   Rect.top = y - 3;
	   Rect.right = x + 3;
	   Rect.bottom = y + 3;
	   FillRect(hDC, &Rect, hbrGray);
	   
	   ReleaseDC(hWnd, hDC);	    
	   break;
         }
        case WM_RBUTTONDOWN:
	 {
	   ULONG x, y;
	   RECT Rect;
	   hDC = GetDC(hWnd);
	   x = LOWORD(lParam);
	   y = HIWORD(lParam);
	    
	   Rect.left = x - 5;
	   Rect.top = y - 5;
	   Rect.right = x + 5;
	   Rect.bottom = y + 5;
	   FillRect(hDC, &Rect, hbrYellow);
	    
	   Rect.left = x - 3;
	   Rect.top = y - 3;
	   Rect.right = x + 3;
	   Rect.bottom = y + 3;
	   FillRect(hDC, &Rect, hbrBlack);
	   
	   ReleaseDC(hWnd, hDC);
	   break;
	 }
        case WM_RBUTTONUP:
	 {
	   ULONG x, y;
	   RECT Rect;
	   hDC = GetDC(hWnd);
	   x = LOWORD(lParam);
	   y = HIWORD(lParam);
	  
	   Rect.left = x - 5;
	   Rect.top = y - 5;
	   Rect.right = x + 5;
	   Rect.bottom = y + 5;
	   FillRect(hDC, &Rect, hbrYellow);
	  	    
	   Rect.left = x - 3;
	   Rect.top = y - 3;
	   Rect.right = x + 3;
	   Rect.bottom = y + 3;
	   FillRect(hDC, &Rect, hbrGray);
	   
	   ReleaseDC(hWnd, hDC);	    
	   break;
	 }
	 
	case WM_MOUSEMOVE:
         {  
          int fwKeys;
          int x;
          int y;
          RECT Rect;
          int temp;
	  TCHAR text[256];

          hDC = GetDC(hWnd);
          fwKeys = wParam;        // key flags 
          x = LOWORD(lParam);  // horizontal position of cursor 
          y = HIWORD(lParam);  // vertical position of cursor 
          
          Rect.left = 10;
	  Rect.top = 100;
	  Rect.right = 160;
	  Rect.bottom = 300;
	  FillRect(hDC, &Rect, hbrWhite);
           
          temp = _sntprintf ( text, sizeof(text)/sizeof(*text), _T("x: %d"), x );
	  TextOut(hDC,10,100,text,strlen(text));
          temp = _sntprintf ( text, sizeof(text)/sizeof(*text), _T("y: %d"), y );
	  TextOut(hDC,10,120,text,strlen(text));

          Rect.left = x - 2;
          Rect.top = y - 2;
          Rect.right = x + 2;
          Rect.bottom = y + 2;
         
          switch ( fwKeys )
          {
                case MK_CONTROL:
                       TextOut(hDC,10,140,"Control",strlen("Control"));       
                       break;
                case MK_SHIFT:
                       TextOut(hDC,10,160,"Shift",strlen("Shift"));                     
                       break;
                case MK_LBUTTON:
                       TextOut(hDC,10,180,"Left",strlen("Left"));       
                       FillRect(hDC, &Rect, hbrRed); 
                       break;
                case MK_MBUTTON:
                       TextOut(hDC,10,200,"Middle",strlen("Middle"));       
	               FillRect(hDC, &Rect, hbrBlue); 
                       break;
                case MK_RBUTTON:
                       TextOut(hDC,10,220,"Right",strlen("Right"));       
	               FillRect(hDC, &Rect, hbrYellow); 
                       break;
           }
                       ReleaseDC(hWnd, hDC);
          break;
         }
         
	case WM_HSCROLL:
	 {
          int nPos;
	  int Pos;
	  int temp;
	  RECT Rect;
	  int nScrollCode;
	  HWND hwndScrollBar;
	  TCHAR text[256];
	  SCROLLINFO Scrollparameter;
 	  nScrollCode = (int) LOWORD(wParam);  // scroll bar value 
	  nPos = (short int) HIWORD(wParam);   // scroll box position 
	  hwndScrollBar = (HWND) lParam;       // handle to scroll bar 
 	  hDC = GetDC(hWnd);
 	  
          Scrollparameter.cbSize = sizeof(Scrollparameter);
          Scrollparameter.fMask = SIF_ALL; 
          GetScrollInfo ( hWnd, SB_HORZ, &Scrollparameter );
          
          Rect.left = 200;
	  Rect.top = 100;
	  Rect.right = 350;
	  Rect.bottom = 300;
	  FillRect(hDC, &Rect, hbrWhite);
 
          switch ( nScrollCode )
          {
                case SB_ENDSCROLL: //Ends scroll. 
                        TextOut(hDC,200,120,"SB_ENDSCROLL    ",16);
	    	        Scrollparameter.nPos = Scrollparameter.nPos;
                        break;
                case SB_LEFT: //Scrolls to the upper left. 
                        TextOut(hDC,200,140,"SB_LEFT         ",16);
	    	        Scrollparameter.nPos = Scrollparameter.nMin;
                        break;
                case SB_RIGHT: //Scrolls to the lower right. 
                        TextOut(hDC,200,160,"SB_RIGHT        ",16);
	    	        Scrollparameter.nPos = Scrollparameter.nMax;
                        break;
                case SB_LINELEFT: //Scrolls left by one unit. 
                        TextOut(hDC,200,180,"SB_LINELEFT     ",16);
	    	        Scrollparameter.nPos--;
                        break;
                case SB_LINERIGHT: //Scrolls right by one unit. 
                        TextOut(hDC,200,200,"SB_LINERIGHT    ",16);
	    	        Scrollparameter.nPos++;
                        break;
                case SB_PAGELEFT: //Scrolls left by the width of the window. 
                        TextOut(hDC,200,220,"SB_PAGELEFT     ",16);
                        Scrollparameter.nPos -= Scrollparameter.nPage;
                        break;
                case SB_PAGERIGHT: //Scrolls right by the width of the window. 
                        TextOut(hDC,200,240,"PAGERIGHT       ",16);
                        Scrollparameter.nPos += Scrollparameter.nPage;
                        break;
                case SB_THUMBPOSITION: //The user has dragged the scroll box (thumb) and released the mouse button. The nPos parameter indicates the position of the scroll box at the end of the drag operation. 
                        TextOut(hDC,200,260,"SB_THUMBPOSITION",16);
	    	        Scrollparameter.nPos = Scrollparameter.nTrackPos;
                        break;
                case SB_THUMBTRACK: //
                        TextOut(hDC,200,280,"SB_THUMBTRACK   ",16);
	    	        Scrollparameter.nPos = Scrollparameter.nTrackPos;
                        break;
          }
          
 	   SetScrollInfo(
                 hWnd,    // handle to window with scroll bar
                 SB_HORZ,    // scroll bar flag
                 &Scrollparameter, // pointer to structure with scroll parameters
                 1 // redraw flag
           );
          temp = _sntprintf ( text, sizeof(text)/sizeof(*text), _T("Horizontal: %d"), Scrollparameter.nPos );
	  TextOut(hDC,200,100,text,strlen(text));
 
 	  return 0;
         }
 	
 	case WM_VSCROLL:
	 {
	  int nPos;
	  int Pos;
	  int temp;
	  RECT Rect;
	  int nScrollCode;
	  HWND hwndScrollBar;
	  TCHAR text[256];
	  SCROLLINFO Scrollparameter;
 	  nScrollCode = (int) LOWORD(wParam);  // scroll bar value 
	  nPos = (short int) HIWORD(wParam);   // scroll box position 
	  hwndScrollBar = (HWND) lParam;       // handle to scroll bar 
 	  hDC = GetDC(hWnd);
 	  
          Scrollparameter.cbSize = sizeof(Scrollparameter);
          Scrollparameter.fMask = SIF_ALL; 
          GetScrollInfo ( hWnd, SB_VERT, &Scrollparameter );
          
          Rect.left = 400;
	  Rect.top = 100;
	  Rect.right = 550;
	  Rect.bottom = 300;
	  FillRect(hDC, &Rect, hbrWhite);
 
          switch ( nScrollCode )
          {
                case SB_ENDSCROLL: //Ends scroll. 
                        TextOut(hDC,400,120,"SB_ENDSCROLL    ",16);
	    	        Scrollparameter.nPos = Scrollparameter.nPos;
	    	        break;
                case SB_LEFT: //Scrolls to the upper left. 
                        TextOut(hDC,400,140,"SB_LEFT         ",16);
	    	        Scrollparameter.nPos = Scrollparameter.nMin;
                        break;
                case SB_RIGHT: //Scrolls to the lower right. 
                        TextOut(hDC,400,160,"SB_RIGHT        ",16);
	    	        Scrollparameter.nPos = Scrollparameter.nMax;
                        break;
                case SB_LINELEFT: //Scrolls left by one unit. 
                        TextOut(hDC,400,180,"SB_LINELEFT     ",16);
	    	        Scrollparameter.nPos--;
                        break;
                case SB_LINERIGHT: //Scrolls right by one unit. 
                        TextOut(hDC,400,200,"SB_LINERIGHT    ",16);
	    	        Scrollparameter.nPos++;
                        break;
                case SB_PAGELEFT: //Scrolls left by the width of the window. 
                        TextOut(hDC,400,220,"SB_PAGELEFT     ",16);
                        Scrollparameter.nPos -= Scrollparameter.nPage;
                        break;
                case SB_PAGERIGHT: //Scrolls right by the width of the window. 
                        TextOut(hDC,400,240,"PAGERIGHT       ",16);
                        Scrollparameter.nPos += Scrollparameter.nPage;
                        break;
                case SB_THUMBPOSITION: //The user has dragged the scroll box (thumb) and released the mouse button. The nPos parameter indicates the position of the scroll box at the end of the drag operation. 
                        TextOut(hDC,400,260,"SB_THUMBPOSITION",16);
	    	        Scrollparameter.nPos = Scrollparameter.nTrackPos;
                        break;
                case SB_THUMBTRACK: //
                        TextOut(hDC,400,280,"SB_THUMBTRACK   ",16);
	    	        Scrollparameter.nPos = Scrollparameter.nTrackPos;
                        break;
          }
          
 	  SetScrollInfo(
                 hWnd,    // handle to window with scroll bar
                 SB_VERT,    // scroll bar flag
                 &Scrollparameter, // pointer to structure with scroll parameters
                 1 // redraw flag
          );
          temp = _sntprintf ( text, sizeof(text)/sizeof(*text), _T("Vertical: %d"), Scrollparameter.nPos );
	  TextOut(hDC,400,100,text,strlen(text));
 
 	  return 0;
 	  }
	case WM_DESTROY:
	  PostQuitMessage(0);
	  DeleteObject ( hbrWhite );
	  DeleteObject ( hbrGray );
	  DeleteObject ( hbrBlack );
	  DeleteObject ( hbrRed );
	  DeleteObject ( hbrBlue );
	  DeleteObject ( hbrYellow );
	  break;

	case WM_CREATE:
	 {
	  hbrWhite = CreateSolidBrush ( RGB(0xFF, 0xFF, 0xFF));
	  hbrGray = CreateSolidBrush ( RGB(0xAF, 0xAF, 0xAF));
	  hbrBlack = CreateSolidBrush ( RGB(0x00, 0x00, 0x00));
	  hbrRed = CreateSolidBrush ( RGB(0xFF, 0x00, 0x00));
	  hbrBlue = CreateSolidBrush ( RGB(0x00, 0x00, 0xFF));
	  hbrYellow = CreateSolidBrush ( RGB(0xFF, 0xFF, 0x00));

	  SCROLLINFO si;
	  si.cbSize = sizeof(si);
	  si.fMask = SIF_ALL;
          si.nMin = 0; 
          si.nMax = 100; 
          si.nPage = 5; 
	  si.nPos = 0;
	  
	  SetScrollInfo ( hWnd, SB_HORZ, &si, FALSE );
	  SetScrollInfo ( hWnd, SB_VERT, &si, FALSE );
	  
	  return 0;
	 }
	 case WM_PAINT:
	  hDC = BeginPaint(hWnd, &ps);
	  EndPaint(hWnd, &ps);
	  //SelectObject(hDC, tf);
	  hDC = GetDC(hWnd);
	  TextOut(hDC, 10, 10, "Hello World from ReactOS!", strlen("Hello World from ReactOS!"));
          GetClientRect(hWnd, &clr);
          GetWindowRect(hWnd, &wir);
          sprintf(spr, "%d,%d,%d,%d              ", clr.left, clr.top, clr.right, clr.bottom);
          sprintf(sir, "%d,%d,%d,%d              ", wir.left, wir.top, wir.right, wir.bottom);
          TextOut(hDC, 10, 30, spr, 20);
          TextOut(hDC, 10, 50, sir, 20);
	  ReleaseDC ( hWnd, hDC );
	  break;
  
	default:
	  return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}


