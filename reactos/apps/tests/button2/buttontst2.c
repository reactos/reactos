#include <windows.h>
#include <stdio.h>

HFONT tf;
LRESULT WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI 
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  WNDCLASS wc;
  MSG msg;
  HWND hWnd;
  HWND hbtn[26];

  wc.lpszClassName = "ButtonTest";
  wc.lpfnWndProc = MainWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
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

  hWnd = CreateWindow("ButtonTest",
		      "Button Test",
		      WS_OVERLAPPEDWINDOW,
		      0,
		      0,
		      CW_USEDEFAULT,
		      CW_USEDEFAULT,
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

  hbtn[0] = CreateWindow(
    "BUTTON","BS_DEFPUSHBUTTON",WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
      10, 10, 200, 40, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

  hbtn[1] = CreateWindow(
    "BUTTON","BS_3STATE",WS_VISIBLE | WS_CHILD | BS_3STATE,
      10, 60, 200, 20, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

  hbtn[2] = CreateWindow(
    "BUTTON","BS_AUTO3STATE",WS_VISIBLE | WS_CHILD | BS_AUTO3STATE,
      10, 90, 200, 20, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

  hbtn[3] = CreateWindow(
    "BUTTON","BS_AUTOCHECKBOX",WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
      10, 120, 200, 20, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

  hbtn[4] = CreateWindow(
    "BUTTON","BS_AUTORADIOBUTTON",WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
      10, 150, 200, 20, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

  hbtn[5] = CreateWindow(
    "BUTTON","BS_CHECKBOX",WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
      10, 180, 200, 20, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

  hbtn[6] = CreateWindow(
    "BUTTON","BS_GROUPBOX",WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
      10, 210, 200, 80, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

  hbtn[7] = CreateWindow(
    "BUTTON","BS_PUSHBUTTON",WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
      20, 230, 180, 30, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

  hbtn[8] = CreateWindow(
    "BUTTON","BS_RADIOBUTTON",WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON,
      10, 300, 200, 20, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

  hbtn[9] = CreateWindow(
    "BUTTON","BS_AUTORADIOBUTTON",WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
      220, 160, 200, 20, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

  hbtn[10] = CreateWindow(
    "BUTTON","BS_DEFPUSHBUTTON|BS_BOTTOM",WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_BOTTOM,
      220, 10, 250, 40, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

  hbtn[11] = CreateWindow(
    "BUTTON","BS_DEFPUSHBUTTON|BS_LEFT",WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_LEFT,
      480, 10, 250, 40, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);
 
  hbtn[12] = CreateWindow(
    "BUTTON","BS_DEFPUSHBUTTON|BS_RIGHT|BS_MULTILINE",WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_RIGHT |BS_MULTILINE,
      740, 10, 150, 60, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

   hbtn[13] = CreateWindow(
    "BUTTON","BS_AUTORADIOBUTTON|BS_TOP",WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | BS_TOP,
      220, 60, 200, 60, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

   // Other Combinations

   hbtn[14] = CreateWindow(
    "BUTTON","BS_AUTORADIOBUTTON|BS_BOTTOM|BS_MULTILINE",WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | BS_BOTTOM | BS_MULTILINE,
      480, 60, 200, 60, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

   hbtn[15] = CreateWindow(
    "BUTTON","BS_AUTORADIOBUTTON|BS_LEFT",WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | BS_LEFT,
      740, 80, 200, 20, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

   hbtn[16] = CreateWindow(
    "BUTTON","BS_AUTORADIOBUTTON|BS_RIGHT|BS_TOP",WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | BS_RIGHT | BS_TOP,
      220, 130, 200, 20, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

   hbtn[17] = CreateWindow(
    "BUTTON","BS_AUTORADIOBUTTON|BS_TOP|BS_MULTILINE",WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | BS_TOP| BS_MULTILINE,
      480, 130, 200, 60, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

   hbtn[18] = CreateWindow(
    "BUTTON","BS_AUTOCHECKBOX|BS_BOTTOM|BS_MULTILINE",WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_BOTTOM | BS_MULTILINE,
      740, 130, 200, 60, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

   hbtn[19] = CreateWindow(
    "BUTTON","BS_AUTOCHECKBOX|BS_TOP|BS_MULTILINE",WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_TOP | BS_MULTILINE,
      480, 190, 200, 60, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

   hbtn[20] = CreateWindow(
    "BUTTON","BS_AUTOCHECKBOX|BS_LEFT|BS_MULTILINE",WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_LEFT | BS_MULTILINE,
      220, 230, 200, 60, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

   hbtn[21] = CreateWindow(
    "BUTTON","BS_AUTOCHECKBOX|BS_RIGHT|BS_MULTILINE",WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_RIGHT | BS_MULTILINE,
      480, 240, 200, 60, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

   hbtn[22] = CreateWindow(
    "BUTTON","BS_GROUPBOX|BS_TOP",WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_TOP,
      10, 340, 200, 60, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

   hbtn[23] = CreateWindow(
    "BUTTON","BS_GROUPBOX|BS_BOTTOM",WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_BOTTOM,
      10, 410, 200, 60, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

   hbtn[24] = CreateWindow(
    "BUTTON","BS_GROUPBOXBOX|BS_LEFT",WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_LEFT,
      520, 340, 200, 60, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

   hbtn[25] = CreateWindow(
    "BUTTON","BS_GROUPBOX|BS_RIGHT|BS_BOTTOM",WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_BOTTOM | BS_RIGHT,
      300, 340, 200, 60, hWnd, NULL, (HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE),NULL);

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

	switch(msg)
	{
	case WM_PAINT:
          hDC = BeginPaint(hWnd, &ps);
	  SelectObject(hDC, tf);
	  EndPaint(hWnd, &ps);
	  break;

	case WM_DESTROY:
	  PostQuitMessage(0);
	  break;

        case WM_COMMAND:
          switch(HIWORD(wParam))
           {
          case BN_CLICKED:
            printf("BUTTON CLICKED !\n");
            break;
          case BN_DBLCLK:
            printf("BUTTON DOUBLE-CLICKED !\n");
            break;
          case BN_PUSHED:
            printf("BUTTON PUSHED !\n");
            break;
          case BN_PAINT:
            printf("BUTTON PAINTED !\n");
            break;
          case BN_UNPUSHED:
            printf("BUTTON UNPUSHED !\n");
            break;

           }
	  break;

	default:
	  return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
