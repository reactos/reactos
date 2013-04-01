#ifndef __CMD_PRECOMP_H
#define __CMD_PRECOMP_H

#ifdef _MSC_VER
#pragma warning ( disable : 4103 ) /* use #pragma pack to change alignment */
#undef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif /*_MSC_VER */

#include <stdlib.h>
#include <malloc.h>

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <shellapi.h>
#include <stdio.h>

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

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

#endif /* __CMD_PRECOMP_H */
