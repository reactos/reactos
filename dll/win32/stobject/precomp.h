#pragma once

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <tchar.h>

#define COBJMACROS
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <wincon.h>
#include <ddeml.h>
#include <shlguid_undoc.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlwapi_undoc.h>
#include <tchar.h>
#include <strsafe.h>
#include <atlbase.h>
#include <atlcom.h>

#include <wine/debug.h>
#include <wine/unicode.h>

extern const GUID CLSID_SysTray;

#include "resource.h"

static __inline ULONG
Win32DbgPrint(const char *filename, int line, const char *lpFormat, ...)
{
    char szMsg[512];
    char *szMsgStart;
    const char *fname;
    va_list vl;
    ULONG uRet;

    fname = strrchr(filename, '\\');
    if (fname == NULL)
    {
        fname = strrchr(filename, '/');
    }

    if (fname == NULL)
        fname = filename;
    else
        fname++;

    szMsgStart = szMsg + sprintf(szMsg, "[%10lu] %s:%d: ", GetTickCount(), fname, line);

    va_start(vl, lpFormat);
    uRet = (ULONG) vsprintf(szMsgStart, lpFormat, vl);
    va_end(vl);

    OutputDebugStringA(szMsg);

    return uRet;
}

#define DbgPrint(fmt, ...) \
    Win32DbgPrint(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#if 1
#define FAILED_UNEXPECTEDLY(hr) (FAILED(hr) && (DbgPrint("Unexpected failure %08x.\n", hr), TRUE))
#else
#define FAILED_UNEXPECTEDLY(hr) FAILED(hr)
#endif

