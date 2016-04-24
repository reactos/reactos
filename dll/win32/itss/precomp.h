#ifndef _ITSS_PCH_
#define _ITSS_PCH_

#include <wine/config.h>
#include <wine/port.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <objbase.h>

#include <wine/itss.h>
#include <wine/unicode.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(itss);

#include "chm_lib.h"
#include "itsstor.h"
#include "lzx.h"

#endif /* _ITSS_PCH_ */
