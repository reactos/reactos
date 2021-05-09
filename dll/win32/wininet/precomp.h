
#pragma once

#include <assert.h>
#include <stdio.h>
#include <wchar.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <kefuncs.h>

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <winnls.h>
#include <wininet.h>
#include <winnetwk.h>

#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_GDI
#include <shlwapi.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <dhcpcsdk.h>
#include <shlobj.h>
#include <shellapi.h>

#include <cryptuiapi.h>

#include <wine/debug.h>
#include <wine/exception.h>

#include "internet.h"
#include "resource.h"

/* msvcrt/ucrtbase incompatibilities */
#define swprintf _snwprintf
