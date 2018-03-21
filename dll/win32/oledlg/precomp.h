
#ifndef _OLEDLG_PRECOMP_H_
#define _OLEDLG_PRECOMP_H_

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <oledlg.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#include "oledlg_private.h"
#include "resource.h"

#endif /* !_OLEDLG_PRECOMP_H_ */
