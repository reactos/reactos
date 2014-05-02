#ifndef _SCHANNEL_PCH_
#define _SCHANNEL_PCH_

#include <stdarg.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <sspi.h>
#include <ntsecapi.h>
#include <ntsecpkg.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(schannel);

#endif /* _SCHANNEL_PCH_ */
