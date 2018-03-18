
#ifndef _MSCTF_PRECOMP_H
#define _MSCTF_PRECOMP_H

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <objbase.h>
#include <olectl.h>
#include <msctf.h>
#include <shlwapi.h>

#include <wine/list.h>
#include <wine/unicode.h>
#include <wine/debug.h>

#include "msctf_internal.h"

#endif /* !_MSCTF_PRECOMP_H */
