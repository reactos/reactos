#ifndef _MSIMTF_PCH_
#define _MSIMTF_PCH_

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <objbase.h>
#include <dimm.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msimtf);

#endif /* _MSIMTF_PCH_ */
