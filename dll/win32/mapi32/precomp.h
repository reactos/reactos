#ifndef _MAPI32_PCH_
#define _MAPI32_PCH_

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <mapi.h>
#include <mapival.h>
#include <shlwapi.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#include "util.h"

WINE_DEFAULT_DEBUG_CHANNEL(mapi);

#endif /* _MAPI32_PCH_ */
