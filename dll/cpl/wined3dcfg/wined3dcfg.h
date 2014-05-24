#ifndef _WINED3DCFG_PCH_
#define _WINED3DCFG_PCH_

#include <stdarg.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <commctrl.h>

#include "resource.h"

#define MAX_KEY_LENGTH 256

#define VALUE_GDI                  L"gdi"
#define VALUE_ENABLED              L"enabled"
#define VALUE_DISABLED             L"disabled"
#define VALUE_NONE                 L"none"
#define VALUE_BACKBUFFER           L"backbuffer"
#define VALUE_FBO                  L"fbo"
#define VALUE_DEFAULT              L"default"

#define KEY_WINE                  L"Software\\Wine\\Direct3D"

#define KEY_GLSL                  L"UseGLSL"
#define KEY_GSLEVEL               L"MaxShaderModelGS"
#define KEY_VSLEVEL               L"MaxShaderModelVS"
#define KEY_PSLEVEL               L"MaxShaderModelPS"
#define KEY_STRICTDRAWORDERING    L"StrictDrawOrdering"
#define KEY_OFFSCREEN             L"OffscreenRenderingMode"
#define KEY_MULTISAMPLING         L"Multisampling"
#define KEY_VIDMEMSIZE            L"VideoMemorySize"
#define KEY_ALWAYSOFFSCREEN       L"AlwaysOffscreen"
#define KEY_DDRENDERER            L"DirectDrawRenderer"

#define INIT_CONTROL(a, b) InitControl(hWndDlg, hKey, KEY_##a, b, IDC_##a, sizeof(b)/sizeof(WINED3D_SETTINGS))
#define SAVE_CONTROL(a, b) SaveSetting(hWndDlg, hKey, KEY_##a, b, IDC_##a, sizeof(b)/sizeof(WINED3D_SETTINGS))

INT_PTR CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef struct _WINED3D_SETTINGS{
    WCHAR szValue[24];
    INT iType;
    INT iValue;
} WINED3D_SETTINGS, *PWINED3D_SETTINGS;

#endif /* _WINED3DCFG_PCH_ */
