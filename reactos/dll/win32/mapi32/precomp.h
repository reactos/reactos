#ifndef _MAPI32_PCH_
#define _MAPI32_PCH_

#include <config.h>

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
#include <initguid.h>
#include <mapival.h>
#include <mapiutil.h>

#include <wine/unicode.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(mapi);

#include "util.h"

#endif /* _MAPI32_PCH_ */
