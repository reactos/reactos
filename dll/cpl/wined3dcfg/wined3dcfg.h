#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <stdio.h>
#include <initguid.h>
#include <debug.h>

#include "resource.h"

#define MAX_KEY_LENGTH 256

#define ITEM_FBO                  0
#define ITEM_BACKBUFFER           1

#define ITEM_READTEX              0
#define ITEM_READDRAW             1
#define ITEM_DISABLED             2

#define VALUE_READTEX              L"readtex"
#define VALUE_READDRAW             L"readdraw"
#define VALUE_ENABLED              L"enabled"
#define VALUE_DISABLED             L"disabled"
#define VALUE_NONE                 L"none"
#define VALUE_BACKBUFFER           L"backbuffer"
#define VALUE_FBO                  L"fbo"

#define KEY_WINE                  L"Software\\Wine\\Direct3D"

#define KEY_GLSL                  L"UseGLSL"
#define KEY_VERTEXSHADERS         L"VertexShaderMode"
#define KEY_PIXELSHADERS          L"PixelShaderMode"
#define KEY_STRICTDRAWORDERING    L"StrictDrawOrdering"
#define KEY_OFFSCREEN             L"OffscreenRenderingMode"
#define KEY_MULTISAMPLING         L"Multisampling"
#define KEY_LOCKING               L"RenderTargetLockMode"

INT_PTR CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
