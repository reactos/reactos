#ifndef _SCHEDSVC_PCH_
#define _SCHEDSVC_PCH_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winsvc.h>

#include <ndk/rtlfuncs.h>

#include <atsvc_s.h>

#include <wine/debug.h>

extern LIST_ENTRY JobListHead;
extern RTL_RESOURCE JobListLock;

DWORD
WINAPI
RpcThreadRoutine(
    LPVOID lpParameter);

#endif /* _SCHEDSVC_PCH_ */
