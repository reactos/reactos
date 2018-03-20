
#ifndef _MSXML3_PCH_
#define _MSXML3_PCH_

#include <config.h>

#ifdef HAVE_LIBXML2
# include <libxml/parser.h>
#endif

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#define COBJMACROS
#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <ole2.h>
#include <msxml6.h>
#include <wininet.h>
#include <shlwapi.h>

#include <wine/debug.h>
#include <wine/list.h>

#include "msxml_private.h"

#endif /* !_MSXML3_PCH_ */
