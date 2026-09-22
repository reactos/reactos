/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Precompiled Header for ReactOS Management Console
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller
 *              Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#ifndef _MMC_PCH_
#define _MMC_PCH_

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <commctrl.h>
#include <commdlg.h>
#include <atlstr.h>
#include <atlwin.h>
#include <atlcom.h>
#include <atlcoll.h>
#include <atlsimpcoll.h>
#include <ui/rosctrls.h>
#include <shlwapi.h>
#include <wincon.h>
#include <shlobj.h>
#include <reactos/debug.h>
#include <shellutils.h>
#include <shellapi.h>
#include <msxml2.h>
#include <mmc.h>

#define WM_USER_CLOSE_CHILD (WM_USER + 1)



#include "resource.h"

typedef enum _DOCUMENT_MODE
{
    DocumentMode_Author = 0,
    DocumentMode_User,
    DocumentMode_UserMDI,
    DocumentMode_UserSDI
} DOCUMENT_MODE, *PDOCUMENT_MODE;

#include "mscfile.h"

#include "CRecentFileEntry.h"
#include "CSnapinCacheEntry.h"
#include "CSnapin.h"
#include "CConsoleWnd.h"
#include "CMainWnd.h"

#include "CAddDialog.h"
#include "CCustomizeDialog.h"
#include "COptionsDialog.h"

#endif /* _MMC_PCH_ */
