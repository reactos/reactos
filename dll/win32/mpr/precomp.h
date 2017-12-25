#ifndef _MPR_PCH_
#define _MPR_PCH_

#include <config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <winnetwk.h>
#include <npapi.h>

#include <wine/debug.h>

#include "mprres.h"

WINE_DEFAULT_DEBUG_CHANNEL(mpr);

#endif /* _MPR_PCH_ */
