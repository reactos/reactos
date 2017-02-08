#ifndef _WKSSVC_PCH_
#define _WKSSVC_PCH_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winsvc.h>

#include <wkssvc_s.h>

#include <wine/debug.h>

DWORD
WINAPI
RpcThreadRoutine(
    LPVOID lpParameter);

#endif /* _WKSSVC_PCH_ */
