#ifndef _SHELLFIND_PCH_
#define _SHELLFIND_PCH_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shdeprecated.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlsimpcoll.h>
#include <atlstr.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <undocshell.h>
#include <shellutils.h>
#include <strsafe.h>
#include <wine/debug.h>

#include "../resource.h"

#define WM_SEARCH_START          WM_USER + 0
#define WM_SEARCH_STOP           WM_USER + 1
#define WM_SEARCH_ADD_RESULT     WM_USER + 2
#define WM_SEARCH_UPDATE_STATUS  WM_USER + 3

struct SearchStart
{
    WCHAR szPath[MAX_PATH];
    WCHAR szFileName[MAX_PATH];
    WCHAR szQuery[MAX_PATH];
    BOOL  SearchHidden;
};

#endif /* _SHELLFIND_PCH_ */
