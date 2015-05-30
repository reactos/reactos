#pragma once

#include <stdio.h>
#include <tchar.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windows.h>
#include <windowsx.h>

#include <setupapi.h>
#include <cfgmgr32.h>
#include <commctrl.h>
#include <Uxtheme.h>
#include <Cfgmgr32.h>
#include <devguid.h>

#include <atlbase.h>

#include <strsafe.h>

#include "resource.h"

extern HINSTANCE g_hInstance;
extern HANDLE ProcessHeap;
