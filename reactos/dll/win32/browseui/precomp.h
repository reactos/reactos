#ifndef _BROWSEUI_PCH_
#define _BROWSEUI_PCH_

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <shlobj.h>
#include <tlogstg.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shdeprecated.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <perhist.h>
#include <exdispid.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <undocshell.h>
#include <wine/debug.h>

#include "resource.h"

#include "aclmulti.h"
#include "addressband.h"
#include "addresseditbox.h"
#include "bandproxy.h"
#include "bandsite.h"
#include "bandsitemenu.h"
#include "brandband.h"
#include "internettoolbar.h"
#include "commonbrowser.h"
#include "globalfoldersettings.h"
#include "regtreeoptions.h"
#include <stdio.h>

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

#endif /* _BROWSEUI_PCH_ */
