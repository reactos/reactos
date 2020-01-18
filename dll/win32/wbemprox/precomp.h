
#ifndef _WBEMPROX_PRECOMP_H_
#define _WBEMPROX_PRECOMP_H_

#include <stdarg.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winsvc.h>
#include <objbase.h>
#include <oleauto.h>
#include <wbemcli.h>

#include "wbemprox_private.h"

#endif /* !_WBEMPROX_PRECOMP_H_ */
