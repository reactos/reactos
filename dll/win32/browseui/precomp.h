#ifndef _BROWSEUI_PCH_
#define _BROWSEUI_PCH_

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <shlobj.h>
#include <tlogstg.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shdeprecated.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <perhist.h>
#include <exdispid.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <wine/debug.h>

#include "resource.h"

#include "aclmulti.h"
#include "addressband.h"
#include "addresseditbox.h"
#include "bandproxy.h"
#include "bandsite.h"
#include "bandsitemenu.h"
#include "brandband.h"
#include "internettoolbar.h"
#include "commonbrowser.h"
#include "globalfoldersettings.h"
#include "regtreeoptions.h"
#include <stdio.h>

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
        if (fname != NULL)
            fname++;
    }
    else
        fname++;

    if (fname == NULL)
        fname = filename;

    szMsgStart = szMsg + sprintf(szMsg, "%s:%d: ", fname, line);

    va_start(vl, lpFormat);
    uRet = (ULONG) vsprintf(szMsgStart, lpFormat, vl);
    va_end(vl);

    OutputDebugStringA(szMsg);

    return uRet;
}

#define DbgPrint(fmt, ...) \
    Win32DbgPrint(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

#endif /* _BROWSEUI_PCH_ */
