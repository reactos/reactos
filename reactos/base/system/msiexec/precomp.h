#ifndef _MSIEXEC_PCH_
#define _MSIEXEC_PCH_

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winsvc.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msiexec);

#endif /* _MSIEXEC_PCH_ */
