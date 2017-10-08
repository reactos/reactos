#ifndef _OPENGLCFG_PCH_
#define _OPENGLCFG_PCH_

#include <stdarg.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <commctrl.h>
#include <strsafe.h>

#include "resource.h"

#define MAX_KEY_LENGTH 256

#define RENDERER_DEFAULT 0
#define RENDERER_RSWR    1

#define DEBUG_SET   1
#define DEBUG_CLEAR 2

#define KEY_RENDERER L"Software\\ReactOS\\OpenGL"
#define KEY_DRIVERS  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers"
#define KEY_DEBUG_CHANNEL L"System\\CurrentControlSet\\Control\\Session Manager\\Environment"

INT_PTR CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

extern HINSTANCE hApplet;

#endif /* _OPENGLCFG_PCH_ */
