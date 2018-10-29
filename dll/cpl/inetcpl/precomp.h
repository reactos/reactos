
#ifndef _INETCPL_PRECOMP_H_
#define _INETCPL_PRECOMP_H_

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define CONST_VTABLE
#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <ole2.h>
#include <commctrl.h>
#include <shlwapi.h>

#include <wine/debug.h>
#include <wine/heap.h>

#include "inetcpl.h"

#endif /* !_INETCPL_PRECOMP_H_ */
