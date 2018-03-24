
#ifndef _WBEMDISP_PRECOMP_H_
#define _WBEMDISP_PRECOMP_H_

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <oleauto.h>
#include <wbemdisp.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#include "wbemdisp_private.h"

#endif /* !_WBEMDISP_PRECOMP_H_ */
