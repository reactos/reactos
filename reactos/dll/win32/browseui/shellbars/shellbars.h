#ifndef _SHELLBARS_PCH_
#define _SHELLBARS_PCH_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <wingdi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shdeprecated.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <undocuser.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <undocshell.h>
#include <shellutils.h>
#include <browseui_undoc.h>
#include <wine/debug.h>

#include "../resource.h"

#include "CBandSite.h"
#include "CBandSiteMenu.h"

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

#endif /* _BROWSEUI_PCH_ */
