#ifndef __CMD_PRECOMP_H
#define __CMD_PRECOMP_H

#ifdef _MSC_VER
#pragma warning ( disable : 4103 ) /* use #pragma pack to change alignment */
#undef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif /*_MSC_VER */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <tchar.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <winuser.h>
#include <wincon.h>
#include <shellapi.h>

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

#include <strsafe.h>

#include <conutils.h>

#include "resource.h"

#include "cmd.h"
#include "config.h"
#include "batch.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(cmd);

#ifdef UNICODE
#define debugstr_aw debugstr_w
#else
#define debugstr_aw debugstr_a
#endif

#ifdef FEATURE_DYNAMIC_TRACE

extern BOOL g_bDynamicTrace;
void CmdTrace(INT type, LPCSTR file, INT line, LPCSTR func, LPCSTR fmt, ...);

#undef FIXME
#define FIXME(fmt, ...) \
    CmdTrace(__WINE_DBCL_FIXME, __FILE__, __LINE__, __FUNCTION__, fmt, ## __VA_ARGS__)

#undef ERR
#define ERR(fmt, ...) \
    CmdTrace(__WINE_DBCL_ERR, __FILE__, __LINE__, __FUNCTION__, fmt, ## __VA_ARGS__)

#undef WARN
#define WARN(fmt, ...) \
    CmdTrace(__WINE_DBCL_WARN, __FILE__, __LINE__, __FUNCTION__, fmt, ## __VA_ARGS__)

#undef TRACE
#define TRACE(fmt, ...) \
    CmdTrace(__WINE_DBCL_TRACE, __FILE__, __LINE__, __FUNCTION__, fmt, ## __VA_ARGS__)

#endif /* def FEATURE_DYNAMIC_TRACE */

#endif /* __CMD_PRECOMP_H */
