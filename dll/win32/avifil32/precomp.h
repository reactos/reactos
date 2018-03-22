
#ifndef _AVIFILE_PRECOMP_H
#define _AVIFILE_PRECOMP_H

#include <assert.h>
#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <wincon.h>
#include <vfw.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#include "avifile_private.h"
#include "extrachunk.h"

#endif /* !_AVIFILE_PRECOMP_H */
