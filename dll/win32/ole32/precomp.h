
#ifndef _OLE32_PCH_
#define _OLE32_PCH_

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#define _INC_WINDOWS

#define COBJMACROS

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#define USE_COM_CONTEXT_DEF
#include <ole2.h>
#include <ole2ver.h>
#include <dcom.h>
#include <comcat.h>
#include <servprov.h>
#include <winternl.h>

#include <wine/debug.h>
#include <wine/list.h>

#include "compobj_private.h"
#include "dictionary.h"
#include "moniker.h"

#endif /* !_OLE32_PCH_ */
