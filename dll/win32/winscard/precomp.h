#ifndef _WINSCARD_PCH_
#define _WINSCARD_PCH_

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winscard.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(winscard);

#endif /* _WINSCARD_PCH_ */
