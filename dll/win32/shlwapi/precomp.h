
#ifndef _SHLWAPI_PCH_
#define _SHLWAPI_PCH_

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <wincon.h>
#include <winternl.h>
#define NO_SHLWAPI_USER
#include <shlwapi.h>
#include <shlobj.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#ifdef __REACTOS__
EXTERN_C HRESULT VariantChangeTypeForRead(_Inout_ VARIANTARG *pvarg, _In_ VARTYPE vt);
#endif

#include "resource.h"

#endif /* !_SHLWAPI_PCH_ */
