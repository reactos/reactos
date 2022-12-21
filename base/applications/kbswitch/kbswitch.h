#pragma once

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winnls.h>
#include <winreg.h>
#include <wingdi.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi_undoc.h>

#include <stdlib.h>
#include <tchar.h>
#include <strsafe.h>

#include "resource.h"

#define KBSWITCH_CLASS L"kbswitcher"

#define WM_LANGUAGE             (WM_USER + 100)
#define WM_WINDOWACTIVATED      (WM_USER + 101)
#define WM_WINDOWCREATED        (WM_USER + 102)
#define WM_WINDOWDESTROYED      (WM_USER + 103)
#define WM_WINDOWSETFOCUS       (WM_USER + 104)
