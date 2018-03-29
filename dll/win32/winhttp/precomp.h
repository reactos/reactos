
#ifndef _WINHTTP_PRECOMP_H_
#define _WINHTTP_PRECOMP_H_

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <ole2.h>
#include <winsock2.h>
#include <winhttp.h>

#include <wine/debug.h>

#include "winhttp_private.h"

#endif /* !_WINHTTP_PRECOMP_H_ */
