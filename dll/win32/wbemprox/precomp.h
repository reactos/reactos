
#ifndef _WBEMPROX_PRECOMP_H_
#define _WBEMPROX_PRECOMP_H_

#include <stdarg.h>
#include <wchar.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winsvc.h>
#include <objbase.h>
#include <oleauto.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <tlhelp32.h>

#include <iads.h>

#include <winternl.h>
#include <winioctl.h>
#include <winsvc.h>
#include <winver.h>
#include <sddl.h>
#include <ntsecapi.h>
#include <winreg.h>
#include <winspool.h>
#include <setupapi.h>

#include <rpcproxy.h>

#include <wine/asm.h>

#include "wbemprox_private.h"

#endif /* !_WBEMPROX_PRECOMP_H_ */
