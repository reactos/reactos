#ifndef _SHELLBARS_PCH_
#define _SHELLBARS_PCH_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shdeprecated.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlsimpcoll.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <undocshell.h>
#include <shellutils.h>
#include <strsafe.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(browseui);

#include "../resource.h"

#include "CBandSite.h"
#include "CBandSiteMenu.h"
#include "CISFBand.h"

#define USE_SYSTEM_ISFBAND 0

#if USE_SYSTEM_ISFBAND
#define CISFBand_CreateInstance(riid, ppv) (CoCreateInstance(CLSID_ISFBand, NULL, CLSCTX_INPROC_SERVER,riid, ppv))
#else
#define CISFBand_CreateInstance RSHELL_CISFBand_CreateInstance
#endif

#endif /* _SHELLBARS_PCH_ */
