#ifndef _BROWSEUI_PCH_
#define _BROWSEUI_PCH_

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <wingdi.h>
#include <shlobj.h>
#include <tlogstg.h>
#include <shellapi.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shdeprecated.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <undocuser.h>
#include <perhist.h>
#include <exdispid.h>
#include <strsafe.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <undocshell.h>
#include <shellutils.h>
#include <browseui_undoc.h>
#include <wine/debug.h>

#include "resource.h"

#include "aclistisf.h"
#include "aclmulti.h"
#include "addressband.h"
#include "addresseditbox.h"
#include "CAutoComplete.h"
#include "bandproxy.h"
#include "shellbars/CBandSite.h"
#include "shellbars/CBandSiteMenu.h"
#include "brandband.h"
#include "internettoolbar.h"
#include "commonbrowser.h"
#include "globalfoldersettings.h"
#include "regtreeoptions.h"
#include "explorerband.h"
#include "CProgressDialog.h"
#include "browseui.h"
#include <stdio.h>

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

#endif /* _BROWSEUI_PCH_ */
