#ifndef _OLEAUT32_PCH_
#define _OLEAUT32_PCH_

#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <ole2.h>
#include <olectl.h>

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/unicode.h>

#include "connpt.h"
#include "variant.h"
#include "resource.h"

#endif /* _OLEAUT32_PCH_ */
