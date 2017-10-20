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
#include <tchar.h>
#include <atlstr.h>
#include <atlwin.h>
#include <atlcom.h>
#include <atlsimpcoll.h>
#include <rosctrls.h>

#include <mmc.h>

//#define NDEBUG
#include <debug.h>

#define WM_USER_CLOSE_CHILD (WM_USER + 1)

// tmp:
#include <shlwapi.h>


#include "resource.h"

#include "CImageList.h"
#include "CSnapin.h"
#include "CConsoleWnd.h"
#include "CAddDialog.h"
#include "CMainWnd.h"


#endif /* _MMC_PCH_ */
