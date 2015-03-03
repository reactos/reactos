#ifndef _WLANSVC_PCH_
#define _WLANSVC_PCH_

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winsvc.h>
#include <wlansvc_s.h>

#include <ndk/rtlfuncs.h>
#include <ndk/obfuncs.h>

typedef struct _WLANSVCHANDLE
{
    LIST_ENTRY WlanSvcHandleListEntry;
    DWORD      dwClientVersion;
} WLANSVCHANDLE, *PWLANSVCHANDLE;

#endif /* _WLANSVC_PCH_ */
