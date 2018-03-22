
#ifndef _PROPSYS_PRECOMP_H_
#define _PROPSYS_PRECOMP_H_

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <propsys.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#include "propsys_private.h"

#endif /* !_PROPSYS_PRECOMP_H_ */
