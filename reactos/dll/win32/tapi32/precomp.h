#ifndef _TAPI32_PCH_
#define _TAPI32_PCH_

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <objbase.h>
#include <tapi.h>

#include <wine/unicode.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(tapi);

#endif /* _TAPI32_PCH_ */
