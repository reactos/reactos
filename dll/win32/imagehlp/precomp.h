#ifndef _IMAGEHLP_PCH_
#define _IMAGEHLP_PCH_

#include <stdarg.h>

#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>
#include <winternl.h>
#include <imagehlp.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

#endif /* _IMAGEHLP_PCH_ */
