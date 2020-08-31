
#ifndef _ATL_PCH_
#define _ATL_PCH_

#include <stdio.h>
#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <objbase.h>
#include <ole2.h>
#include <oleauto.h>
#include <exdisp.h>
#include <shlwapi.h>

#include <wine/atlbase.h>
#include <wine/atlcom.h>
#include <wine/atlwin.h>
#include <wine/debug.h>
#include <wine/unicode.h>
#include <wine/heap.h>

#endif /* !_ATL_PCH_ */
