#include <windows.h>

#include "resource.h"
#include "display.h"

#define MAX_LOADSTRING 50
#define MAX_BUTTONNAME 30

#define HEADER_SIZE 37
#define BUTTON_POS_X 6
#define BUTTON_POS_Y 8
#define BUTTON_WIDTH 72
#define BUTTON_HEIGHT 21

#define IDC_QUIT 1001
#define IDC_PRINT 1002
#define IDC_DISPLAY 1003

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);

BOOL LoadFont(LPWSTR lpCmdLine);
